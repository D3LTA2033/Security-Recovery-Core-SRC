/**
 * Legacy Motherboard Support
 * 
 * Provides compatibility for older motherboards and legacy BIOS systems
 */

#ifndef LEGACY_SUPPORT_H
#define LEGACY_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>

/* Legacy motherboard types */
typedef enum {
    LEGACY_TYPE_UNKNOWN = 0,
    LEGACY_TYPE_BIOS_LEGACY,      // Traditional BIOS (pre-UEFI)
    LEGACY_TYPE_UEFI_LEGACY,      // Early UEFI (UEFI 2.0-2.3)
    LEGACY_TYPE_SMALL_FLASH,       // 4MB or 8MB SPI flash
    LEGACY_TYPE_NO_EC,             // No embedded controller
    LEGACY_TYPE_OLD_SPI,           // Older SPI flash chips (pre-2010)
    LEGACY_TYPE_LPC_ONLY,          // LPC bus only (no modern SPI)
    LEGACY_TYPE_CUSTOM              // Custom/embedded board
} legacy_motherboard_type_t;

/* Legacy detection structure */
typedef struct {
    legacy_motherboard_type_t type;
    bool has_ec;
    bool has_tpm;
    bool has_secure_boot;
    uint32_t flash_size;
    uint32_t flash_sector_size;
    bool supports_large_sectors;   // 64KB sectors vs 4KB only
    bool supports_write_protect;
    uint8_t spi_interface_type;    // 0=SPI, 1=LPC, 2=parallel
    bool legacy_bios_mode;
    uint32_t boot_timeout_ms;      // Adjusted for legacy systems
} legacy_board_info_t;

/* Function prototypes */

/**
 * Detect legacy motherboard type and capabilities
 */
bool legacy_detect_motherboard(legacy_board_info_t *info);

/**
 * Initialize SPI flash with legacy compatibility
 */
bool legacy_spi_init(const legacy_board_info_t *info);

/**
 * Read from SPI flash with legacy fallback methods
 */
bool legacy_spi_read(uint32_t offset, uint8_t *buffer, size_t size, 
                     const legacy_board_info_t *info);

/**
 * Write to SPI flash with legacy compatibility
 */
bool legacy_spi_write(uint32_t offset, const uint8_t *buffer, size_t size,
                      const legacy_board_info_t *info);

/**
 * Erase sector with legacy size support
 */
bool legacy_spi_erase(uint32_t offset, const legacy_board_info_t *info);

/**
 * Detect SPI flash size (supports older chips)
 */
uint32_t legacy_detect_flash_size(void);

/**
 * Get adjusted boot timeout for legacy systems
 */
uint32_t legacy_get_boot_timeout(const legacy_board_info_t *info);

/**
 * Check if legacy BIOS mode is active
 */
bool legacy_is_bios_mode(void);

/**
 * Initialize legacy boot detection (POST codes, etc.)
 */
bool legacy_boot_detection_init(const legacy_board_info_t *info);

/**
 * Read legacy POST codes (port 0x80)
 */
uint8_t legacy_read_post_code(void);

/**
 * Check for legacy watchdog timer
 */
bool legacy_has_watchdog(void);

/**
 * Initialize legacy USB (USB 1.1 fallback)
 */
bool legacy_usb_init(const legacy_board_info_t *info);

/**
 * Get legacy-compatible SRC region offset
 * Older boards may need different offset
 */
uint32_t legacy_get_src_region_offset(const legacy_board_info_t *info);

/**
 * Get legacy-compatible SRC region size
 * May be smaller on older boards
 */
uint32_t legacy_get_src_region_size(const legacy_board_info_t *info);

/**
 * Check if board supports hardware write protection
 */
bool legacy_supports_write_protect(const legacy_board_info_t *info);

/**
 * Apply legacy-specific workarounds
 */
void legacy_apply_workarounds(legacy_board_info_t *info);

#endif /* LEGACY_SUPPORT_H */


