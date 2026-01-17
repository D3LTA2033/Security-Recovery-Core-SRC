/**
 * Enhanced Recovery Mechanisms
 * 
 * Provides advanced recovery capabilities including multiple USB detection,
 * recovery priority, verification, and health monitoring
 */

#ifndef ENHANCED_RECOVERY_H
#define ENHANCED_RECOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Maximum number of USB devices to check */
#define MAX_USB_DEVICES 4

/* Recovery priority levels */
typedef enum {
    RECOVERY_PRIORITY_LOW = 0,
    RECOVERY_PRIORITY_NORMAL = 1,
    RECOVERY_PRIORITY_HIGH = 2,
    RECOVERY_PRIORITY_CRITICAL = 3
} recovery_priority_t;

/* USB device information */
typedef struct {
    char path[64];
    bool present;
    bool valid_structure;
    bool has_backup_a;
    bool has_backup_b;
    bool has_manifest;
    bool has_signature;
    uint32_t backup_a_size;
    uint32_t backup_b_size;
    uint32_t last_modified;
    recovery_priority_t priority;
} usb_device_info_t;

/* Recovery verification result */
typedef struct {
    bool success;
    bool signature_valid;
    bool structure_valid;
    bool firmware_valid;
    uint8_t firmware_hash[32];
    char error_message[128];
} recovery_verification_t;

/* Health check result */
typedef struct {
    bool system_healthy;
    bool firmware_valid;
    bool config_valid;
    bool backups_valid;
    bool usb_available;
    uint32_t last_backup_age;
    uint32_t last_recovery_age;
    uint8_t health_score;  // 0-100
    char issues[256];
} health_status_t;

/* Function prototypes */

/**
 * Scan for multiple USB devices with recovery structure
 */
uint32_t enhanced_scan_usb_devices(usb_device_info_t *devices, uint32_t max_devices);

/**
 * Select best USB device for recovery based on priority and validity
 */
bool enhanced_select_best_usb(usb_device_info_t *devices, uint32_t count, 
                               usb_device_info_t **selected);

/**
 * Perform recovery with verification
 */
bool enhanced_recover_with_verification(const usb_device_info_t *device, 
                                       recovery_verification_t *verification);

/**
 * Verify recovery after restore
 */
bool enhanced_verify_recovery(const uint8_t *firmware, size_t size,
                              recovery_verification_t *verification);

/**
 * Perform comprehensive health check
 */
bool enhanced_health_check(health_status_t *status);

/**
 * Monitor firmware integrity continuously
 */
bool enhanced_monitor_integrity(void);

/**
 * Get recovery priority for current situation
 */
recovery_priority_t enhanced_get_recovery_priority(void);

/**
 * Check if multiple recovery sources are available
 */
bool enhanced_has_multiple_sources(void);

/**
 * Perform automatic recovery from best available source
 */
bool enhanced_auto_recover(void);

/**
 * Verify backup integrity before use
 */
bool enhanced_verify_backup(const char *backup_path, bool *is_valid);

/**
 * Get detailed recovery statistics
 */
bool enhanced_get_recovery_stats(uint32_t *total_recoveries, 
                                 uint32_t *successful_recoveries,
                                 uint32_t *failed_recoveries,
                                 uint32_t *last_recovery_timestamp);

#endif /* ENHANCED_RECOVERY_H */

