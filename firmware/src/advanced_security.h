/**
 * Advanced Security Features
 * 
 * Provides TPM integration, secure boot verification, enhanced tamper detection,
 * and firmware integrity monitoring
 */

#ifndef ADVANCED_SECURITY_H
#define ADVANCED_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* TPM support */
typedef struct {
    bool available;
    bool initialized;
    uint8_t tpm_version;  // 1 = TPM 1.2, 2 = TPM 2.0
    bool has_nvram;
} tpm_info_t;

/* Secure boot status */
typedef struct {
    bool enabled;
    bool verified;
    uint8_t boot_mode;  // 0 = disabled, 1 = setup mode, 2 = user mode, 3 = deployed
    char boot_policy[64];
} secure_boot_status_t;

/* Tamper detection result */
typedef struct {
    bool tamper_detected;
    uint32_t tamper_timestamp;
    uint8_t tamper_type;  // 0 = none, 1 = config, 2 = firmware, 3 = logs, 4 = hardware
    char tamper_details[128];
} tamper_detection_t;

/* Integrity monitoring result */
typedef struct {
    bool integrity_ok;
    uint32_t last_check_timestamp;
    uint8_t firmware_hash[32];
    uint8_t config_hash[32];
    bool hash_match;
} integrity_status_t;

/* Function prototypes */

/**
 * Initialize TPM support
 */
bool security_tpm_init(tpm_info_t *info);

/**
 * Store firmware hash in TPM NVRAM
 */
bool security_tpm_store_hash(const uint8_t *hash, size_t hash_size);

/**
 * Verify firmware hash from TPM NVRAM
 */
bool security_tpm_verify_hash(const uint8_t *hash, size_t hash_size);

/**
 * Check secure boot status
 */
bool security_check_secure_boot(secure_boot_status_t *status);

/**
 * Verify secure boot chain
 */
bool security_verify_boot_chain(void);

/**
 * Detect tampering attempts
 */
bool security_detect_tampering(tamper_detection_t *detection);

/**
 * Monitor firmware integrity
 */
bool security_monitor_integrity(integrity_status_t *status);

/**
 * Perform cryptographic attestation
 */
bool security_perform_attestation(uint8_t *attestation_data, size_t *data_size);

/**
 * Verify attestation data
 */
bool security_verify_attestation(const uint8_t *attestation_data, size_t data_size);

/**
 * Enable hardware write protection
 */
bool security_enable_write_protect(void);

/**
 * Check if write protection is active
 */
bool security_is_write_protected(void);

/**
 * Perform security audit
 */
bool security_perform_audit(char *audit_report, size_t report_size);

/**
 * Get security status summary
 */
bool security_get_status_summary(char *summary, size_t summary_size);

#endif /* ADVANCED_SECURITY_H */

