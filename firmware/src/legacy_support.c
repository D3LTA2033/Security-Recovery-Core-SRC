/**
 * Legacy Motherboard Support Implementation
 * 
 * Provides compatibility for older motherboards, legacy BIOS systems,
 * and older SPI flash chips
 */

#include "legacy_support.h"
#include "platform.h"
#include "spi_flash.h"
#include <string.h>

/* Known legacy motherboard signatures */
static const struct {
    const char *signature;
    legacy_motherboard_type_t type;
    uint32_t flash_size;
    bool has_ec;
} legacy_boards[] = {
    /* Legacy BIOS boards */
    {"AWARD", LEGACY_TYPE_BIOS_LEGACY, 4 * 1024 * 1024, false},
    {"AMI", LEGACY_TYPE_BIOS_LEGACY, 4 * 1024 * 1024, false},
    {"PHOENIX", LEGACY_TYPE_BIOS_LEGACY, 4 * 1024 * 1024, false},
    
    /* Early UEFI boards */
    {"INSYDE", LEGACY_TYPE_UEFI_LEGACY, 8 * 1024 * 1024, true},
    {"AMI_UEFI", LEGACY_TYPE_UEFI_LEGACY, 8 * 1024 * 1024, true},
    
    /* Small flash boards */
    {"SMALL_FLASH", LEGACY_TYPE_SMALL_FLASH, 4 * 1024 * 1024, false},
    
    /* LPC-only boards (very old) */
    {"LPC_ONLY", LEGACY_TYPE_LPC_ONLY, 2 * 1024 * 1024, false},
    
    {NULL, LEGACY_TYPE_UNKNOWN, 0, false}
};

/**
 * Detect legacy motherboard type
 */
bool legacy_detect_motherboard(legacy_board_info_t *info) {
    if (!info) {
        return false;
    }
    
    memset(info, 0, sizeof(legacy_board_info_t));
    
    /* Default to unknown */
    info->type = LEGACY_TYPE_UNKNOWN;
    info->flash_size = 16 * 1024 * 1024;  // Default 16MB
    info->flash_sector_size = 4096;      // Default 4KB sectors
    info->boot_timeout_ms = 45000;       // Longer timeout for legacy (45s)
    info->spi_interface_type = 0;        // Default SPI
    
    /* Try to detect flash size */
    uint32_t detected_size = legacy_detect_flash_size();
    if (detected_size > 0) {
        info->flash_size = detected_size;
        
        /* Small flash indicates legacy board */
        if (detected_size <= 4 * 1024 * 1024) {
            info->type = LEGACY_TYPE_SMALL_FLASH;
        } else if (detected_size <= 8 * 1024 * 1024) {
            info->type = LEGACY_TYPE_UEFI_LEGACY;
        }
    }
    
    /* Check for legacy BIOS */
    if (legacy_is_bios_mode()) {
        info->legacy_bios_mode = true;
        if (info->type == LEGACY_TYPE_UNKNOWN) {
            info->type = LEGACY_TYPE_BIOS_LEGACY;
        }
    }
    
    /* Check for EC (Embedded Controller) */
    /* Platform-specific detection */
    info->has_ec = platform_has_ec();
    
    /* Check for TPM */
    info->has_tpm = platform_has_tpm();
    
    /* Detect sector size */
    /* Try to detect if board supports 64KB sectors */
    info->supports_large_sectors = platform_supports_large_sectors();
    if (!info->supports_large_sectors) {
        info->flash_sector_size = 4096;  // Force 4KB sectors
    } else {
        info->flash_sector_size = 65536; // 64KB sectors
    }
    
    /* Check write protection support */
    info->supports_write_protect = platform_supports_write_protect();
    
    /* Adjust SRC region for small flash */
    if (info->flash_size <= 4 * 1024 * 1024) {
        /* Use smaller region on 4MB flash */
        info->type = LEGACY_TYPE_SMALL_FLASH;
    }
    
    /* Apply workarounds */
    legacy_apply_workarounds(info);
    
    return true;
}

/**
 * Initialize SPI flash with legacy compatibility
 */
bool legacy_spi_init(const legacy_board_info_t *info) {
    if (!info) {
        return spi_flash_init();
    }
    
    /* For LPC-only boards, use LPC interface */
    if (info->spi_interface_type == 1) {
        return platform_lpc_init();
    }
    
    /* Standard SPI initialization */
    bool result = spi_flash_init();
    
    if (!result && info->type == LEGACY_TYPE_OLD_SPI) {
        /* Try legacy SPI initialization */
        return platform_legacy_spi_init();
    }
    
    return result;
}

/**
 * Read from SPI flash with legacy fallback
 */
bool legacy_spi_read(uint32_t offset, uint8_t *buffer, size_t size,
                     const legacy_board_info_t *info) {
    if (!info) {
        return spi_flash_read(offset, buffer, size);
    }
    
    /* Check bounds for small flash */
    if (offset + size > info->flash_size) {
        return false;
    }
    
    /* Use appropriate interface */
    if (info->spi_interface_type == 1) {
        return platform_lpc_read(offset, buffer, size);
    }
    
    return spi_flash_read(offset, buffer, size);
}

/**
 * Write to SPI flash with legacy compatibility
 */
bool legacy_spi_write(uint32_t offset, const uint8_t *buffer, size_t size,
                      const legacy_board_info_t *info) {
    if (!info) {
        return spi_flash_write(offset, buffer, size);
    }
    
    /* Check bounds */
    if (offset + size > info->flash_size) {
        return false;
    }
    
    /* Use appropriate sector size */
    uint32_t sector_size = info->flash_sector_size;
    
    /* Erase sector before write if needed */
    uint32_t sector_start = (offset / sector_size) * sector_size;
    if (offset % sector_size == 0) {
        if (!legacy_spi_erase(sector_start, info)) {
            return false;
        }
    }
    
    /* Use appropriate interface */
    if (info->spi_interface_type == 1) {
        return platform_lpc_write(offset, buffer, size);
    }
    
    return spi_flash_write(offset, buffer, size);
}

/**
 * Erase sector with legacy size support
 */
bool legacy_spi_erase(uint32_t offset, const legacy_board_info_t *info) {
    if (!info) {
        return spi_flash_erase_sector(offset);
    }
    
    /* Use appropriate sector size */
    uint32_t sector_size = info->flash_sector_size;
    uint32_t sector_start = (offset / sector_size) * sector_size;
    
    if (info->spi_interface_type == 1) {
        return platform_lpc_erase(sector_start);
    }
    
    return spi_flash_erase_sector(sector_start);
}

/**
 * Detect SPI flash size (supports older chips)
 */
uint32_t legacy_detect_flash_size(void) {
    /* Try standard detection first */
    uint32_t size = spi_flash_get_size();
    if (size > 0) {
        return size;
    }
    
    /* Try legacy detection methods */
    /* Read JEDEC ID and look up size */
    uint8_t jedec_id[3];
    if (platform_read_jedec_id(jedec_id)) {
        /* Look up size from JEDEC ID */
        return platform_get_size_from_jedec(jedec_id);
    }
    
    /* Fallback: try reading from known locations */
    /* Some older chips have size info at specific offsets */
    uint32_t detected = platform_detect_flash_size_legacy();
    if (detected > 0) {
        return detected;
    }
    
    /* Default to 4MB for unknown legacy boards */
    return 4 * 1024 * 1024;
}

/**
 * Get adjusted boot timeout for legacy systems
 */
uint32_t legacy_get_boot_timeout(const legacy_board_info_t *info) {
    if (!info) {
        return 30000;  // Default 30s
    }
    
    /* Legacy BIOS systems often boot slower */
    if (info->legacy_bios_mode) {
        return 60000;  // 60s for legacy BIOS
    }
    
    if (info->type == LEGACY_TYPE_BIOS_LEGACY) {
        return 50000;  // 50s for legacy BIOS
    }
    
    if (info->type == LEGACY_TYPE_UEFI_LEGACY) {
        return 40000;  // 40s for early UEFI
    }
    
    return info->boot_timeout_ms;
}

/**
 * Check if legacy BIOS mode is active
 */
bool legacy_is_bios_mode(void) {
    /* Check for legacy BIOS signatures */
    /* Read from BIOS region */
    uint8_t signature[4];
    
    /* Check for "AWARD" or "AMI" signatures */
    if (platform_read_bios_signature(signature)) {
        if (memcmp(signature, "AWAR", 4) == 0 ||
            memcmp(signature, "AMI", 3) == 0 ||
            memcmp(signature, "PHNX", 4) == 0) {
            return true;
        }
    }
    
    /* Check UEFI presence */
    /* If no UEFI, assume legacy BIOS */
    return !platform_has_uefi();
}

/**
 * Initialize legacy boot detection
 */
bool legacy_boot_detection_init(const legacy_board_info_t *info) {
    if (!info) {
        return false;
    }
    
    /* For legacy BIOS, use POST code monitoring */
    if (info->legacy_bios_mode || info->type == LEGACY_TYPE_BIOS_LEGACY) {
        return platform_init_post_code_monitoring();
    }
    
    /* Standard boot detection */
    return platform_boot_detection_init();
}

/**
 * Read legacy POST codes
 */
uint8_t legacy_read_post_code(void) {
    return platform_read_post_code();
}

/**
 * Check for legacy watchdog
 */
bool legacy_has_watchdog(void) {
    return platform_has_watchdog();
}

/**
 * Initialize legacy USB
 */
bool legacy_usb_init(const legacy_board_info_t *info) {
    if (!info) {
        return platform_usb_init();
    }
    
    /* Try USB 2.0 first */
    bool result = platform_usb_init();
    
    if (!result && info->type == LEGACY_TYPE_BIOS_LEGACY) {
        /* Fallback to USB 1.1 for very old boards */
        return platform_usb_init_legacy();
    }
    
    return result;
}

/**
 * Get legacy-compatible SRC region offset
 */
uint32_t legacy_get_src_region_offset(const legacy_board_info_t *info) {
    if (!info) {
        return 0x100000;  // Default 1MB offset
    }
    
    /* For small flash, use smaller offset */
    if (info->flash_size <= 4 * 1024 * 1024) {
        return 0x300000;  // 3MB offset for 4MB flash
    }
    
    if (info->flash_size <= 8 * 1024 * 1024) {
        return 0x600000;  // 6MB offset for 8MB flash
    }
    
    return 0x100000;  // Default 1MB offset
}

/**
 * Get legacy-compatible SRC region size
 */
uint32_t legacy_get_src_region_size(const legacy_board_info_t *info) {
    if (!info) {
        return 512 * 1024;  // Default 512KB
    }
    
    /* For small flash, use smaller region */
    if (info->flash_size <= 4 * 1024 * 1024) {
        return 256 * 1024;  // 256KB for 4MB flash
    }
    
    if (info->flash_size <= 8 * 1024 * 1024) {
        return 384 * 1024;  // 384KB for 8MB flash
    }
    
    return 512 * 1024;  // Default 512KB
}

/**
 * Check if board supports write protection
 */
bool legacy_supports_write_protect(const legacy_board_info_t *info) {
    if (!info) {
        return true;  // Assume support
    }
    
    return info->supports_write_protect;
}

/**
 * Apply legacy-specific workarounds
 */
void legacy_apply_workarounds(legacy_board_info_t *info) {
    if (!info) {
        return;
    }
    
    /* Workaround for boards without EC */
    if (!info->has_ec && info->type == LEGACY_TYPE_NO_EC) {
        /* Use alternative boot detection */
        info->boot_timeout_ms += 10000;  // Add 10s timeout
    }
    
    /* Workaround for old SPI chips */
    if (info->type == LEGACY_TYPE_OLD_SPI) {
        /* Force 4KB sectors */
        info->flash_sector_size = 4096;
        info->supports_large_sectors = false;
    }
    
    /* Workaround for LPC-only boards */
    if (info->type == LEGACY_TYPE_LPC_ONLY) {
        info->spi_interface_type = 1;  // Use LPC
        info->boot_timeout_ms = 60000;  // Longer timeout
    }
}


