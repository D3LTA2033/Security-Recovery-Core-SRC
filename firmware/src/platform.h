/**
 * Platform Abstraction Layer
 * Platform-specific implementations must provide these functions
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* SPI Flash */
bool platform_spi_init(void);
bool platform_spi_read(uint32_t offset, uint8_t *buffer, size_t size);
bool platform_spi_write(uint32_t offset, const uint8_t *buffer, size_t size);
bool platform_spi_erase(uint32_t offset);
bool platform_spi_lock(void);
bool platform_spi_unlock(void);
uint32_t platform_spi_get_size(void);

/* USB Mass Storage */
bool platform_usb_init(void);
bool platform_usb_is_present(void);
bool platform_usb_read_file(const char *path, uint8_t *buffer, size_t *size);
bool platform_usb_write_file(const char *path, const uint8_t *buffer, size_t size);
bool platform_usb_delete_file(const char *path);
bool platform_usb_file_exists(const char *path);
bool platform_usb_rename_file(const char *old_path, const char *new_path);

/* Boot Detection */
void platform_boot_detection_init(void);

/* Crypto */
bool platform_crypto_init(void);
void platform_sha256(const uint8_t *data, size_t size, uint8_t *hash);
bool platform_sign(const uint8_t *data, size_t size, 
                  uint8_t *signature, size_t *sig_size);
bool platform_verify(const uint8_t *data, size_t size, 
                    const uint8_t *signature, size_t sig_size);

/* System */
uint32_t platform_get_timestamp(void);
void system_reboot(void);
void src_enter_safe_mode(void);
bool platform_authenticate(void);
void platform_debug_log(const char *message);
void platform_init(void);
void platform_delay_ms(uint32_t ms);

/* Legacy motherboard support functions */
bool platform_has_ec(void);
bool platform_has_tpm(void);
bool platform_has_uefi(void);
bool platform_has_watchdog(void);
bool platform_supports_large_sectors(void);
bool platform_supports_write_protect(void);
bool platform_read_bios_signature(uint8_t *signature);
bool platform_init_post_code_monitoring(void);
uint8_t platform_read_post_code(void);
bool platform_lpc_init(void);
bool platform_lpc_read(uint32_t offset, uint8_t *buffer, size_t size);
bool platform_lpc_write(uint32_t offset, const uint8_t *buffer, size_t size);
bool platform_lpc_erase(uint32_t offset);
bool platform_legacy_spi_init(void);
bool platform_read_jedec_id(uint8_t *id);
uint32_t platform_get_size_from_jedec(const uint8_t *jedec_id);
uint32_t platform_detect_flash_size_legacy(void);
bool platform_usb_init_legacy(void);

/* Advanced security platform functions */
bool platform_secure_boot_enabled(void);
uint8_t platform_get_secure_boot_mode(void);
bool platform_verify_secure_boot_chain(void);
bool platform_get_secure_boot_policy(char *policy, size_t policy_size);
bool platform_detect_hardware_tampering(void);
bool platform_is_spi_locked(void);
uint8_t platform_get_tpm_version(void);
bool platform_tpm_init(void);
bool platform_tpm_has_nvram(void);
bool platform_tpm_nvram_write(uint32_t index, const uint8_t *data, size_t size);
bool platform_tpm_nvram_read(uint32_t index, uint8_t *data, size_t size);

#endif /* PLATFORM_H */
