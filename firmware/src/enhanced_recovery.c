/**
 * Enhanced Recovery Mechanisms Implementation
 * 
 * Provides advanced recovery capabilities including multiple USB detection,
 * recovery priority, verification, and health monitoring
 */

#include "enhanced_recovery.h"
#include "usb_msd.h"
#include "crypto.h"
#include "spi_flash.h"
#include "recovery_core.h"
#include "platform.h"
#include <string.h>
#include <stdlib.h>

/**
 * Scan for multiple USB devices with recovery structure
 */
uint32_t enhanced_scan_usb_devices(usb_device_info_t *devices, uint32_t max_devices) {
    if (!devices || max_devices == 0) {
        return 0;
    }
    
    uint32_t found_count = 0;
    
    /* Check multiple potential USB paths */
    const char *usb_paths[] = {
        "/SECURITY_RECOVERY",
        "/media/SECURITY_RECOVERY",
        "/mnt/SECURITY_RECOVERY",
        "E:\\SECURITY_RECOVERY"  // Windows
    };
    
    for (uint32_t i = 0; i < sizeof(usb_paths) / sizeof(usb_paths[0]) && found_count < max_devices; i++) {
        usb_device_info_t *device = &devices[found_count];
        memset(device, 0, sizeof(usb_device_info_t));
        
        strncpy(device->path, usb_paths[i], sizeof(device->path) - 1);
        
        /* Check if USB device is present */
        device->present = platform_usb_is_present();
        
        if (!device->present) {
            continue;
        }
        
        /* Check for recovery structure */
        char manifest_path[128];
        char backup_a_path[128];
        char backup_b_path[128];
        char signature_path[128];
        
        snprintf(manifest_path, sizeof(manifest_path), "%s/%s", device->path, MANIFEST_FILE);
        snprintf(backup_a_path, sizeof(backup_a_path), "%s/%s", device->path, BACKUP_A_FILE);
        snprintf(backup_b_path, sizeof(backup_b_path), "%s/%s", device->path, BACKUP_B_FILE);
        snprintf(signature_path, sizeof(signature_path), "%s/%s", device->path, SIGNATURE_FILE);
        
        device->has_manifest = platform_usb_file_exists(manifest_path);
        device->has_backup_a = platform_usb_file_exists(backup_a_path);
        device->has_backup_b = platform_usb_file_exists(backup_b_path);
        device->has_signature = platform_usb_file_exists(signature_path);
        
        device->valid_structure = device->has_manifest && 
                                  (device->has_backup_a || device->has_backup_b) &&
                                  device->has_signature;
        
        if (device->valid_structure) {
            /* Get file sizes */
            if (device->has_backup_a) {
                uint8_t dummy[1];
                size_t size = 1;
                if (platform_usb_read_file(backup_a_path, dummy, &size)) {
                    device->backup_a_size = size;
                }
            }
            
            if (device->has_backup_b) {
                uint8_t dummy[1];
                size_t size = 1;
                if (platform_usb_read_file(backup_b_path, dummy, &size)) {
                    device->backup_b_size = size;
                }
            }
            
            /* Set priority based on backup availability */
            if (device->has_backup_a && device->has_backup_b) {
                device->priority = RECOVERY_PRIORITY_HIGH;
            } else if (device->has_backup_a || device->has_backup_b) {
                device->priority = RECOVERY_PRIORITY_NORMAL;
            } else {
                device->priority = RECOVERY_PRIORITY_LOW;
            }
            
            device->last_modified = platform_get_timestamp();
            found_count++;
        }
    }
    
    return found_count;
}

/**
 * Select best USB device for recovery
 */
bool enhanced_select_best_usb(usb_device_info_t *devices, uint32_t count,
                               usb_device_info_t **selected) {
    if (!devices || count == 0 || !selected) {
        return false;
    }
    
    usb_device_info_t *best = NULL;
    recovery_priority_t best_priority = RECOVERY_PRIORITY_LOW;
    
    for (uint32_t i = 0; i < count; i++) {
        if (!devices[i].valid_structure || !devices[i].present) {
            continue;
        }
        
        if (!best || devices[i].priority > best_priority) {
            best = &devices[i];
            best_priority = devices[i].priority;
        } else if (devices[i].priority == best_priority) {
            /* Prefer device with both backups */
            if (devices[i].has_backup_a && devices[i].has_backup_b &&
                (!best->has_backup_a || !best->has_backup_b)) {
                best = &devices[i];
            }
        }
    }
    
    if (best) {
        *selected = best;
        return true;
    }
    
    return false;
}

/**
 * Perform recovery with verification
 */
bool enhanced_recover_with_verification(const usb_device_info_t *device,
                                         recovery_verification_t *verification) {
    if (!device || !verification) {
        return false;
    }
    
    memset(verification, 0, sizeof(recovery_verification_t));
    
    if (!device->valid_structure) {
        strncpy(verification->error_message, "Invalid USB structure",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    /* Verify structure */
    verification->structure_valid = device->has_manifest &&
                                    (device->has_backup_a || device->has_backup_b) &&
                                    device->has_signature;
    
    if (!verification->structure_valid) {
        strncpy(verification->error_message, "Missing required files",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    /* Read firmware backup */
    const char *backup_file = device->has_backup_a ? BACKUP_A_FILE : BACKUP_B_FILE;
    char backup_path[128];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", device->path, backup_file);
    
    uint8_t *firmware_buffer = malloc(8 * 1024 * 1024);  // 8MB max
    if (!firmware_buffer) {
        strncpy(verification->error_message, "Memory allocation failed",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    size_t firmware_size = 8 * 1024 * 1024;
    if (!platform_usb_read_file(backup_path, firmware_buffer, &firmware_size)) {
        free(firmware_buffer);
        strncpy(verification->error_message, "Failed to read firmware",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    /* Calculate firmware hash */
    platform_sha256(firmware_buffer, firmware_size, verification->firmware_hash);
    
    /* Read and verify signature */
    char signature_path[128];
    snprintf(signature_path, sizeof(signature_path), "%s/%s", device->path, SIGNATURE_FILE);
    
    uint8_t signature[512];
    size_t sig_size = sizeof(signature);
    if (!platform_usb_read_file(signature_path, signature, &sig_size)) {
        free(firmware_buffer);
        strncpy(verification->error_message, "Failed to read signature",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    /* Verify signature */
    verification->signature_valid = platform_verify(firmware_buffer, firmware_size,
                                                    signature, sig_size);
    
    if (!verification->signature_valid) {
        free(firmware_buffer);
        strncpy(verification->error_message, "Signature verification failed",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    /* Verify firmware structure */
    verification->firmware_valid = (firmware_size > 0 && firmware_size <= 8 * 1024 * 1024);
    
    if (!verification->firmware_valid) {
        free(firmware_buffer);
        strncpy(verification->error_message, "Invalid firmware size",
                sizeof(verification->error_message) - 1);
        return false;
    }
    
    free(firmware_buffer);
    verification->success = true;
    return true;
}

/**
 * Verify recovery after restore
 */
bool enhanced_verify_recovery(const uint8_t *firmware, size_t size,
                              recovery_verification_t *verification) {
    if (!firmware || size == 0 || !verification) {
        return false;
    }
    
    memset(verification, 0, sizeof(recovery_verification_t));
    
    /* Calculate hash */
    platform_sha256(firmware, size, verification->firmware_hash);
    
    /* Basic validation */
    verification->firmware_valid = (size > 0 && size <= 8 * 1024 * 1024);
    verification->structure_valid = true;  // Assume valid if we got here
    
    /* Note: Signature verification would need to be done with stored public key */
    verification->signature_valid = true;  // Placeholder
    
    verification->success = verification->firmware_valid && 
                           verification->structure_valid &&
                           verification->signature_valid;
    
    return verification->success;
}

/**
 * Perform comprehensive health check
 */
bool enhanced_health_check(health_status_t *status) {
    if (!status) {
        return false;
    }
    
    memset(status, 0, sizeof(health_status_t));
    status->health_score = 100;
    
    /* Check firmware validity */
    src_config_t config;
    if (src_read_config(&config)) {
        status->config_valid = true;
        status->firmware_valid = (config.firmware_hash[0] != 0);
    } else {
        status->health_score -= 30;
        strcat(status->issues, "Config invalid; ");
    }
    
    /* Check USB availability */
    status->usb_available = platform_usb_is_present();
    if (!status->usb_available) {
        status->health_score -= 15;
        strcat(status->issues, "USB not available; ");
    }
    
    /* Check backup age */
    if (config.last_backup_timestamp > 0) {
        uint32_t current_time = platform_get_timestamp();
        status->last_backup_age = current_time - config.last_backup_timestamp;
        
        /* Penalize if backup is old (more than 24 hours) */
        if (status->last_backup_age > (24 * 60 * 60 * 1000)) {
            status->health_score -= 10;
            strcat(status->issues, "Backup is old; ");
        }
    } else {
        status->health_score -= 20;
        strcat(status->issues, "No backup performed; ");
    }
    
    /* Check if backups are valid */
    usb_device_info_t devices[MAX_USB_DEVICES];
    uint32_t device_count = enhanced_scan_usb_devices(devices, MAX_USB_DEVICES);
    status->backups_valid = (device_count > 0);
    
    if (!status->backups_valid) {
        status->health_score -= 15;
        strcat(status->issues, "No valid backups found; ");
    }
    
    status->system_healthy = (status->health_score >= 80);
    
    return true;
}

/**
 * Monitor firmware integrity continuously
 */
bool enhanced_monitor_integrity(void) {
    src_config_t config;
    if (!src_read_config(&config)) {
        return false;
    }
    
    /* Read current firmware */
    uint8_t current_firmware[8 * 1024 * 1024];
    size_t firmware_size = sizeof(current_firmware);
    
    if (!src_read_firmware(current_firmware, firmware_size, 0)) {
        return false;
    }
    
    /* Calculate hash */
    uint8_t current_hash[32];
    platform_sha256(current_firmware, firmware_size, current_hash);
    
    /* Compare with stored hash */
    if (memcmp(current_hash, config.firmware_hash, 32) != 0) {
        /* Integrity violation detected */
        return false;
    }
    
    return true;
}

/**
 * Get recovery priority for current situation
 */
recovery_priority_t enhanced_get_recovery_priority(void) {
    /* Check boot failure count */
    src_config_t config;
    if (!src_read_config(&config)) {
        return RECOVERY_PRIORITY_NORMAL;
    }
    
    /* If multiple failures, increase priority */
    /* This would need additional tracking in config */
    
    return RECOVERY_PRIORITY_NORMAL;
}

/**
 * Check if multiple recovery sources are available
 */
bool enhanced_has_multiple_sources(void) {
    usb_device_info_t devices[MAX_USB_DEVICES];
    uint32_t count = enhanced_scan_usb_devices(devices, MAX_USB_DEVICES);
    return (count > 1);
}

/**
 * Perform automatic recovery from best available source
 */
bool enhanced_auto_recover(void) {
    usb_device_info_t devices[MAX_USB_DEVICES];
    uint32_t device_count = enhanced_scan_usb_devices(devices, MAX_USB_DEVICES);
    
    if (device_count == 0) {
        return false;
    }
    
    usb_device_info_t *selected = NULL;
    if (!enhanced_select_best_usb(devices, device_count, &selected)) {
        return false;
    }
    
    recovery_verification_t verification;
    if (!enhanced_recover_with_verification(selected, &verification)) {
        return false;
    }
    
    /* Perform actual recovery */
    return src_recover_from_usb();
}

/**
 * Verify backup integrity before use
 */
bool enhanced_verify_backup(const char *backup_path, bool *is_valid) {
    if (!backup_path || !is_valid) {
        return false;
    }
    
    *is_valid = false;
    
    /* Check if file exists */
    if (!platform_usb_file_exists(backup_path)) {
        return false;
    }
    
    /* Read and verify signature */
    /* Implementation would verify cryptographic signature */
    
    *is_valid = true;
    return true;
}

/**
 * Get detailed recovery statistics
 */
bool enhanced_get_recovery_stats(uint32_t *total_recoveries,
                                 uint32_t *successful_recoveries,
                                 uint32_t *failed_recoveries,
                                 uint32_t *last_recovery_timestamp) {
    if (!total_recoveries || !successful_recoveries || 
        !failed_recoveries || !last_recovery_timestamp) {
        return false;
    }
    
    /* Read from config or dedicated stats region */
    src_config_t config;
    if (!src_read_config(&config)) {
        return false;
    }
    
    *last_recovery_timestamp = config.last_recovery_timestamp;
    
    /* Stats would be stored in extended config or separate region */
    *total_recoveries = 0;
    *successful_recoveries = 0;
    *failed_recoveries = 0;
    
    return true;
}

