#include <bootloader/bootloader.hpp>
#include <log.h>
#include <sys.h>
#include <data_exchange.hpp>
#include <device/hal.h>
#include <bsod.h>
#include <cmsis_os.h>

using Version = buddy::bootloader::Version;

LOG_COMPONENT_DEF(Bootloader, LOG_SEVERITY_INFO);

Version buddy::bootloader::get_version() {
    Version *const bootloader_version = (Version *)0x0801FFFA;
    return *bootloader_version;
}

/**
 * @brief Part of fw_invalidate that needs to be in RAM.
 * @param sector sector number to erase
 */
[[noreturn]] static void __RAM_FUNC fw_invalidate_ram(const unsigned int sector) {
    // Make sure we have all from flash
    __ISB();
    __DSB();
    ///@note From now on we cannot use any function that is not __RAM_FUNC.

    // Erase the sector
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
    CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
    FLASH->CR |= FLASH_PSIZE_WORD;
    CLEAR_BIT(FLASH->CR, FLASH_CR_SNB);
    FLASH->CR |= FLASH_CR_SER | (sector << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;

    // Wait for last operation to be completed and clear flags
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET)
        ;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
    CLEAR_BIT(FLASH->CR, (FLASH_CR_SER | FLASH_CR_SNB));

    // Cannot continue with anything, firmware is dead now
    sys_reset();
}

// Needs to be RAM function because it erases program memory in flash
bool buddy::bootloader::fw_invalidate(void) {
    if (!data_exchange::is_bootloader_valid()) {
        return false; // Cannot erase firmware if there is no bootloader
    }

    // Flash firmware on next boot
    data_exchange::fw_update_on_restart_enable();

    // Get sector of firmware descriptors, stored right after bootloader and before firmware
    unsigned int sector = bootloader_sector_count;
    if (sector > FLASH_SECTOR_11) {
        sector += 4U; // Need to add offset of 4 when sector higher than FLASH_SECTOR_11
    }

    log_critical(Bootloader, "!!! Bricking itself !!! Erasing sector %i", sector);

    // Disable RTOS and interrupts, the following cannot be interrupted
    osThreadSuspendAll();
    __disable_irq();

    // Unlock flash (don't bother locking afterwards)
    if (HAL_FLASH_Unlock() != HAL_OK) {
        bsod("Failed to unlock flash");
    }

    // Erase one sector
    fw_invalidate_ram(sector);

    // Never reached
}
