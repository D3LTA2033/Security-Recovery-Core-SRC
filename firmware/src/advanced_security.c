/**
 * Advanced Security Features Implementation
 * 
 * Provides TPM integration, secure boot verification, enhanced tamper detection,
 * and firmware integrity monitoring
 */

#include "advanced_security.h"
#include "recovery_core.h"
#include "crypto.h"
#include "platform.h"
#include <string.h>

static tpm_info_t tpm_info_cache = {0};
static bool tpm_initialized = false;

/**
 * Initialize TPM support
 */
bool security_tpm_init(tpm_info_t *info) {
    if (!info) {
        return false;
    }
    
    if (tpm_initialized && tpm_info_cache.available) {
        *info = tpm_info_cache;
        return true;
    }
    
    memset(info, 0, sizeof(tpm_info_t));
    
    /* Check TPM availability */
    info->available = platform_has_tpm();
    
    if (!info->available) {
        return false;
    }
    
    /* Detect TPM version */
    info->tpm_version = platform_get_tpm_version();
    
    /* Initialize TPM */
    if (platform_tpm_init()) {
        info->initialized = true;
        info->has_nvram = platform_tpm_has_nvram();
        
        tpm_info_cache = *info;
        tpm_initialized = true;
        return true;
    }
    
    return false;
}

/**
 * Store firmware hash in TPM NVRAM
 */
bool security_tpm_store_hash(const uint8_t *hash, size_t hash_size) {
    if (!hash || hash_size != 32) {  // SHA-256 is 32 bytes
        return false;
    }
    
    if (!tpm_initialized) {
        tpm_info_t info;
        if (!security_tpm_init(&info) || !info.initialized) {
            return false;
        }
    }
    
    if (!tpm_info_cache.has_nvram) {
        return false;
    }
    
    /* Store hash in TPM NVRAM */
    return platform_tpm_nvram_write(0x1000000, hash, hash_size);
}

/**
 * Verify firmware hash from TPM NVRAM
 */
bool security_tpm_verify_hash(const uint8_t *hash, size_t hash_size) {
    if (!hash || hash_size != 32) {
        return false;
    }
    
    if (!tpm_initialized) {
        tpm_info_t info;
        if (!security_tpm_init(&info) || !info.initialized) {
            return false;
        }
    }
    
    if (!tpm_info_cache.has_nvram) {
        return false;
    }
    
    /* Read stored hash from TPM */
    uint8_t stored_hash[32];
    if (!platform_tpm_nvram_read(0x1000000, stored_hash, sizeof(stored_hash))) {
        return false;
    }
    
    /* Compare hashes */
    return (memcmp(hash, stored_hash, 32) == 0);
}

/**
 * Check secure boot status
 */
bool security_check_secure_boot(secure_boot_status_t *status) {
    if (!status) {
        return false;
    }
    
    memset(status, 0, sizeof(secure_boot_status_t));
    
    /* Check if secure boot is enabled */
    status->enabled = platform_secure_boot_enabled();
    
    if (!status->enabled) {
        return true;  // Not an error, just disabled
    }
    
    /* Get boot mode */
    status->boot_mode = platform_get_secure_boot_mode();
    
    /* Verify boot chain */
    status->verified = platform_verify_secure_boot_chain();
    
    /* Get boot policy */
    platform_get_secure_boot_policy(status->boot_policy, sizeof(status->boot_policy));
    
    return true;
}

/**
 * Verify secure boot chain
 */
bool security_verify_boot_chain(void) {
    secure_boot_status_t status;
    if (!security_check_secure_boot(&status)) {
        return false;
    }
    
    if (!status.enabled) {
        return true;  // Not required if disabled
    }
    
    return status.verified;
}

/**
 * Detect tampering attempts
 */
bool security_detect_tampering(tamper_detection_t *detection) {
    if (!detection) {
        return false;
    }
    
    memset(detection, 0, sizeof(tamper_detection_t));
    
    /* Check config tampering */
    src_config_t config;
    if (!src_read_config(&config)) {
        detection->tamper_detected = true;
        detection->tamper_type = 1;  // Config tampering
        strncpy(detection->tamper_details, "Config read failed or invalid",
                sizeof(detection->tamper_details) - 1);
        detection->tamper_timestamp = platform_get_timestamp();
        return true;
    }
    
    /* Check firmware integrity */
    uint8_t current_hash[32];
    uint8_t stored_hash[32];
    memcpy(stored_hash, config.firmware_hash, 32);
    
    /* Read current firmware and calculate hash */
    uint8_t firmware[8 * 1024 * 1024];
    size_t firmware_size = sizeof(firmware);
    if (src_read_firmware(firmware, firmware_size, 0)) {
        platform_sha256(firmware, firmware_size, current_hash);
        
        if (memcmp(current_hash, stored_hash, 32) != 0) {
            detection->tamper_detected = true;
            detection->tamper_type = 2;  // Firmware tampering
            strncpy(detection->tamper_details, "Firmware hash mismatch",
                    sizeof(detection->tamper_details) - 1);
            detection->tamper_timestamp = platform_get_timestamp();
            return true;
        }
    }
    
    /* Check hardware tampering (if supported) */
    if (platform_detect_hardware_tampering()) {
        detection->tamper_detected = true;
        detection->tamper_type = 4;  // Hardware tampering
        strncpy(detection->tamper_details, "Hardware tampering detected",
                sizeof(detection->tamper_details) - 1);
        detection->tamper_timestamp = platform_get_timestamp();
        return true;
    }
    
    return true;
}

/**
 * Monitor firmware integrity
 */
bool security_monitor_integrity(integrity_status_t *status) {
    if (!status) {
        return false;
    }
    
    memset(status, 0, sizeof(integrity_status_t));
    
    /* Read current firmware */
    uint8_t firmware[8 * 1024 * 1024];
    size_t firmware_size = sizeof(firmware);
    
    if (!src_read_firmware(firmware, firmware_size, 0)) {
        return false;
    }
    
    /* Calculate current hash */
    platform_sha256(firmware, firmware_size, status->firmware_hash);
    
    /* Read stored config */
    src_config_t config;
    if (!src_read_config(&config)) {
        return false;
    }
    
    /* Calculate config hash */
    platform_sha256((uint8_t *)&config, sizeof(config), status->config_hash);
    
    /* Compare firmware hash */
    status->hash_match = (memcmp(status->firmware_hash, config.firmware_hash, 32) == 0);
    
    status->integrity_ok = status->hash_match;
    status->last_check_timestamp = platform_get_timestamp();
    
    return true;
}

/**
 * Perform cryptographic attestation
 */
bool security_perform_attestation(uint8_t *attestation_data, size_t *data_size) {
    if (!attestation_data || !data_size || *data_size < 256) {
        return false;
    }
    
    /* Create attestation structure */
    struct {
        uint32_t timestamp;
        uint8_t firmware_hash[32];
        uint8_t config_hash[32];
        uint8_t signature[128];
    } attestation;
    
    attestation.timestamp = platform_get_timestamp();
    
    /* Get firmware hash */
    src_config_t config;
    if (src_read_config(&config)) {
        memcpy(attestation.firmware_hash, config.firmware_hash, 32);
    }
    
    /* Calculate config hash */
    platform_sha256((uint8_t *)&config, sizeof(config), attestation.config_hash);
    
    /* Sign attestation */
    size_t sig_size = 128;
    if (!platform_sign((uint8_t *)&attestation, sizeof(attestation) - 128,
                       attestation.signature, &sig_size)) {
        return false;
    }
    
    /* Copy to output */
    size_t output_size = sizeof(attestation);
    if (*data_size < output_size) {
        return false;
    }
    
    memcpy(attestation_data, &attestation, output_size);
    *data_size = output_size;
    
    return true;
}

/**
 * Verify attestation data
 */
bool security_verify_attestation(const uint8_t *attestation_data, size_t data_size) {
    if (!attestation_data || data_size < 256) {
        return false;
    }
    
    /* Verify signature */
    return platform_verify(attestation_data, data_size - 128,
                          attestation_data + data_size - 128, 128);
}

/**
 * Enable hardware write protection
 */
bool security_enable_write_protect(void) {
    return platform_spi_lock();
}

/**
 * Check if write protection is active
 */
bool security_is_write_protected(void) {
    return platform_is_spi_locked();
}

/**
 * Perform security audit
 */
bool security_perform_audit(char *audit_report, size_t report_size) {
    if (!audit_report || report_size < 512) {
        return false;
    }
    
    char *ptr = audit_report;
    int remaining = report_size;
    int written;
    
    /* TPM status */
    tpm_info_t tpm_info;
    if (security_tpm_init(&tpm_info)) {
        written = snprintf(ptr, remaining, "TPM: Available (v%d.%d)\n",
                           tpm_info.tpm_version, tpm_info.has_nvram ? 1 : 0);
        ptr += written;
        remaining -= written;
    } else {
        written = snprintf(ptr, remaining, "TPM: Not available\n");
        ptr += written;
        remaining -= written;
    }
    
    /* Secure boot status */
    secure_boot_status_t sb_status;
    if (security_check_secure_boot(&sb_status)) {
        written = snprintf(ptr, remaining, "Secure Boot: %s (%s)\n",
                          sb_status.enabled ? "Enabled" : "Disabled",
                          sb_status.verified ? "Verified" : "Not Verified");
        ptr += written;
        remaining -= written;
    }
    
    /* Integrity status */
    integrity_status_t integrity;
    if (security_monitor_integrity(&integrity)) {
        written = snprintf(ptr, remaining, "Integrity: %s\n",
                          integrity.integrity_ok ? "OK" : "FAILED");
        ptr += written;
        remaining -= written;
    }
    
    /* Tamper detection */
    tamper_detection_t tamper;
    if (security_detect_tampering(&tamper)) {
        written = snprintf(ptr, remaining, "Tampering: %s\n",
                          tamper.tamper_detected ? "DETECTED" : "None");
        ptr += written;
        remaining -= written;
    }
    
    /* Write protection */
    written = snprintf(ptr, remaining, "Write Protection: %s\n",
                      security_is_write_protected() ? "Active" : "Inactive");
    
    return true;
}

/**
 * Get security status summary
 */
bool security_get_status_summary(char *summary, size_t summary_size) {
    if (!summary || summary_size < 256) {
        return false;
    }
    
    /* Build summary */
    integrity_status_t integrity;
    bool integrity_ok = security_monitor_integrity(&integrity);
    
    tamper_detection_t tamper;
    bool tamper_ok = !security_detect_tampering(&tamper) || !tamper.tamper_detected;
    
    bool write_protected = security_is_write_protected();
    
    snprintf(summary, summary_size,
            "Security Status: %s | Integrity: %s | Tampering: %s | Write Protect: %s",
            (integrity_ok && tamper_ok) ? "OK" : "WARNING",
            integrity_ok ? "OK" : "FAILED",
            tamper_ok ? "None" : "DETECTED",
            write_protected ? "Active" : "Inactive");
    
    return true;
}

