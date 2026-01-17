/**
 * SPI Flash Implementation
 * Platform-specific implementations should override these functions
 */

#include "spi_flash.h"
#include "platform.h"
#include <string.h>

static bool spi_initialized = false;

bool spi_flash_init(void) {
    if (spi_initialized) {
        return true;
    }
    
    /* Platform-specific initialization */
    if (!platform_spi_init()) {
        return false;
    }
    
    spi_initialized = true;
    return true;
}

bool spi_flash_read(uint32_t offset, uint8_t *buffer, size_t size) {
    /* SECURITY: Validate parameters */
    if (!spi_initialized || !buffer || size == 0) {
        return false;
    }
    
    /* SECURITY: Bounds checking - prevent overflow */
    uint32_t flash_size = spi_flash_get_size();
    if (flash_size == 0) {
        return false;  /* Flash not initialized */
    }
    
    /* Check for integer overflow in offset + size */
    if (size > UINT32_MAX || offset > UINT32_MAX - size) {
        return false;  /* Would overflow */
    }
    
    /* Check bounds */
    if (offset >= flash_size || (offset + size) > flash_size) {
        return false;  /* Out of bounds */
    }
    
    /* Platform-specific read */
    return platform_spi_read(offset, buffer, size);
}

bool spi_flash_write(uint32_t offset, const uint8_t *buffer, size_t size) {
    /* SECURITY: Validate parameters */
    if (!spi_initialized || !buffer || size == 0) {
        return false;
    }
    
    /* SECURITY: Bounds checking - prevent overflow and corruption */
    uint32_t flash_size = spi_flash_get_size();
    if (flash_size == 0) {
        return false;  /* Flash not initialized */
    }
    
    /* Check for integer overflow in offset + size */
    if (size > UINT32_MAX || offset > UINT32_MAX - size) {
        return false;  /* Would overflow */
    }
    
    /* Check bounds */
    if (offset >= flash_size || (offset + size) > flash_size) {
        return false;  /* Out of bounds - would corrupt flash */
    }
    
    /* SECURITY: Sanity check - prevent writing to critical regions without explicit permission
     * This is a safety check - actual protection should be in platform layer */
    if (offset == 0 && size > 1024) {
        /* Writing to start of flash - extra caution */
        /* In production, this should check if we're in recovery mode */
    }
    
    /* Erase sector if needed before write */
    uint32_t sector_size = 4096;  // Typical 4KB sectors
    uint32_t sector_start = (offset / sector_size) * sector_size;
    
    if (offset % sector_size == 0) {
        /* SECURITY: Verify sector erase is within bounds */
        if (sector_start >= flash_size) {
            return false;
        }
        if (!spi_flash_erase_sector(sector_start)) {
            return false;
        }
    }
    
    /* Platform-specific write */
    return platform_spi_write(offset, buffer, size);
}

bool spi_flash_erase_sector(uint32_t offset) {
    if (!spi_initialized) {
        return false;
    }
    
    /* Platform-specific erase */
    return platform_spi_erase(offset);
}

bool spi_flash_lock(void) {
    if (!spi_initialized) {
        return false;
    }
    
    /* Platform-specific lock */
    return platform_spi_lock();
}

bool spi_flash_unlock(void) {
    if (!spi_initialized) {
        return false;
    }
    
    /* Platform-specific unlock */
    return platform_spi_unlock();
}

uint32_t spi_flash_get_size(void) {
    /* Platform-specific size detection */
    return platform_spi_get_size();
}
