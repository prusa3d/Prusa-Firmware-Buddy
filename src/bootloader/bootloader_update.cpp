#include <logging/log.hpp>
#include "bsod.h"
#include <cstdio>
#include <cerrno>
#include <tuple>
#include <memory>
#include "cmsis_os.h"
#include "crc32.h"
#include "bsod.h"
#include <string.h>
#include <freertos/critical_section.hpp>
#include "sys.h"
#include <span>
#include <random.h>

#include <unique_file_ptr.hpp>

#include "data_exchange.hpp"
#include "resources/bootstrap.hpp"
#include "resources/revision_bootloader.hpp"
#include "bootloader/bootloader.hpp"
#include "bootloader/required_version.hpp"

// FIXME: Those includes are here only for the RNG.
// We should add support for the stdlib's standard random function
#include <buddy/main.h>

LOG_COMPONENT_REF(Bootloader);

#define fatal_error(msg) bsod(msg)

using Version = buddy::bootloader::Version;
using UpdateStage = buddy::bootloader::UpdateStage;

constexpr size_t bootloader_sector_get_size(int sector) {
    return buddy::bootloader::bootloader_sector_sizes[sector];
}

constexpr uintptr_t bootloader_sector_get_address(int sector) {
    uintptr_t base_address = 0x08000000;
    for (int i = 0; i < sector; i++) {
        base_address += bootloader_sector_get_size(i);
    }
    return base_address;
}

static bool calculate_file_crc(FILE *fp, uint32_t length, uint32_t &crc) {
    uint8_t buffer[64];
    while (length) {
        size_t to_read = std::min(sizeof(buffer), (unsigned int)length);
        size_t read = fread(buffer, 1, to_read, fp);
        crc = crc32_calc_ex(crc, buffer, read);
        length -= read;
        if ((feof(fp) && length != 0) || ferror(fp)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static bool flash_erase_sector(int sector) {
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    FLASH_EraseInitTypeDef erase_settings;
    erase_settings.Banks = FLASH_BANK_1;
    erase_settings.Sector = sector;
    erase_settings.NbSectors = 1;
    erase_settings.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_settings.TypeErase = FLASH_TYPEERASE_SECTORS;

    uint32_t failed_at_sector;
    auto status = HAL_FLASHEx_Erase(&erase_settings, &failed_at_sector);
    return status == HAL_OK;
}

static bool flash_program(const uint8_t *flash_address, const uint8_t *data, size_t length) {
    bool success = true;

    freertos::CriticalSection critical_section;

    while (length) {
        uint32_t program_type;
        uint64_t block_data;
        size_t block_length;

        if (length > 8 && false) {
            program_type = FLASH_TYPEPROGRAM_DOUBLEWORD;
            memcpy(&block_data, data, sizeof(uint64_t));
            block_length = sizeof(uint64_t);
        } else {
            program_type = FLASH_TYPEPROGRAM_BYTE;
            memcpy(&block_data, data, sizeof(uint8_t));
            block_length = sizeof(uint8_t);
        }

        if (HAL_FLASH_Program(program_type, (uint32_t)flash_address, block_data) != HAL_OK) {
            success = false;
            break;
        }

        flash_address += block_length;
        length -= block_length;
        data += block_length;
    }

    return success;
}

template <typename ProgressCallback>
static bool flash_program_sector(int sector, FILE *fp, std::span<uint8_t> buffer, ProgressCallback progress) {
    const size_t sector_size = bootloader_sector_get_size(sector);
    const uint8_t *address = reinterpret_cast<const uint8_t *>(bootloader_sector_get_address(sector));
    const uint8_t *next_sector_address = address + sector_size;
    size_t copied_bytes = 0;

    if (sector == 0) {
        // make sure we will update the prebootloader in one go
        assert(buffer.size() >= bootloader_sector_get_size(0));
    }

    while (address < next_sector_address && !feof(fp)) {
        size_t to_read = std::min(buffer.size(), static_cast<size_t>(next_sector_address - address));
        size_t read = fread(buffer.data(), 1, to_read, fp);

        if (ferror(fp)) {
            log_error(Bootloader, "Bootloader reading failed while flashing (errno %i)", errno);
            return false;
        }

        log_debug(Bootloader, "Programming %p (size %zu)", address, read);
        if (!flash_program(address, buffer.data(), read)) {
            log_error(Bootloader, "Writing the bootloader to FLASH failed (errno %i)", errno);
            return false;
        }
        log_debug(Bootloader, "Programming %p (size %zu) finished", address, read);

        address += read;
        copied_bytes += read;

        progress(copied_bytes);
    }

    return true;
}

/// Copy the bootloader from file to FLASH
///
/// The bootloader is located in the first 128 KB of FLASH
/// and consists of the preboot (the first 16 KB) and the main bootloader (the rest).
///
/// The FLASH is organised into sectors and before we write anything, we must erase the whole sector
/// sector0 - 16 KB : stores the preboot
/// sector1 - 16 KB : main bootloader
/// sector2 - 16 KB : main bootloader
/// sector3 - 16 KB : main bootloader
/// sector4 - 64 KB : main bootloader
///
template <typename ProgressCallback>
static void copy_bootloader_to_flash(FILE *bootloader_bin, ProgressCallback progress) {
    const size_t total_bytes = 131072;
    size_t bytes_in_preceding_sectors = 0;

    auto buffer_size = bootloader_sector_get_size(0);
    auto buffer_mem = std::make_unique<uint8_t[]>(buffer_size);
    if (buffer_mem.get() == nullptr) {
        bsod("Not enough memory");
    }
    auto buffer = std::span<uint8_t>(buffer_mem.get(), buffer_size);

    for (unsigned sector = 0; sector < buddy::bootloader::bootloader_sector_count; sector++) {

        // do not reflash preboot if not necessary
        if (sector == 0) {
            if (!buddy::bootloader::preboot_needs_update()) {
                log_info(Bootloader, "No need to update preboot (bootloader 2.x). Skipping sector 0.");
                continue;
            }
            uint32_t expected_preboot_crc = 0;
            if (!calculate_file_crc(bootloader_bin, bootloader_sector_get_size(0), expected_preboot_crc)) {
                fatal_error("expected preboot crc calculation failed");
            }

            uint32_t current_preboot_crc = crc32_calc(reinterpret_cast<const uint8_t *>(bootloader_sector_get_address(0)), bootloader_sector_get_size(0));
            if (current_preboot_crc == expected_preboot_crc) {
                log_info(Bootloader, "No need to update preboot (CRC matches). Skipping sector 0.");
                continue;
            } else {
                log_info(Bootloader, "Going to update preboot now.");
            }
        }

        log_info(Bootloader, "Flashing sector %i", sector);

        // add random delay to make preboot flashing less predictable
        if (sector == 0) {
            uint32_t delay_ms = 100 + (rand_u() % 7000);
            osDelay(delay_ms);
        }

        // seek at the sector in the file
        if (fseek(bootloader_bin, bootloader_sector_get_address(sector) - bootloader_sector_get_address(0), SEEK_SET) != 0) {
            fatal_error("bootloader update: failed to seek sector");
        }

        // erase the sector
        HAL_FLASH_Unlock();
        if (!flash_erase_sector(sector)) {
            fatal_error("bootloader update: failed to erase sector");
        }

        // program the sector
        HAL_FLASH_Unlock();
        bool flash_successful = flash_program_sector(sector, bootloader_bin, buffer, [&](size_t bytes_written) {
            if (sector != 0) {
                // do not report progress for sector 0, as updating preboot is potentially dangerous
                // and we want to be as quick as possible and minimize the code running in-between
                size_t total_written = bytes_in_preceding_sectors + bytes_written;
                int percent_done = 100 * total_written / total_bytes;
                progress(percent_done);
            }
        });
        HAL_FLASH_Lock();

        if (!flash_successful) {
            fatal_error("bootloader update: failed to flash sector");
        }

        log_info(Bootloader, "Sector %i flashed successfully", sector);
        bytes_in_preceding_sectors += bootloader_sector_get_size(sector);
    }
}

bool buddy::bootloader::needs_update(buddy::bootloader::Version required) {
    if (data_exchange::is_bootloader_valid() == false) {
        return true;
    }

    auto current = get_version();

    return std::tie(current.major, current.minor, current.patch)
        < std::tie(required.major, required.minor, required.patch);
}

bool buddy::bootloader::needs_update() {
    return needs_update(buddy::bootloader::required_version);
}

bool buddy::bootloader::preboot_needs_update() {
    return needs_update({ 2, 0, 1 });
}

void buddy::bootloader::update(ProgressHook progress) {
    auto calc_percent_done = [](int bootstrap_percent, int update_percent) {
        return (int)(bootstrap_percent * 0.75 + update_percent * 0.25);
    };

    // get bootloader.bin to the internal flash
    bool needs_bootstrap = buddy::resources::has_resources(buddy::resources::revision::bootloader) == false;
    if (needs_bootstrap) {
        buddy::resources::bootstrap(buddy::resources::revision::bootloader, [&](int percent_done, buddy::resources::BootstrapStage stage) {
            switch (stage) {
            case buddy::resources::BootstrapStage::LookingForBbf:
                progress(0, UpdateStage::LookingForBbf);
                break;
            case buddy::resources::BootstrapStage::PreparingBootstrap:
            case buddy::resources::BootstrapStage::CopyingFiles:
                progress(calc_percent_done(percent_done, 0), buddy::bootloader::UpdateStage::PreparingUpdate);
                break;
            }
        });
    }

    unique_file_ptr bootloader_bin(fopen("/internal/res/bootloader.bin", "rb"));
    if (bootloader_bin.get() == nullptr) {
        log_critical(Bootloader, "bootloader.bin failed to fopen() after its bootstrap");
        fatal_error("bootloader.bin failed to open");
    }

    progress(calc_percent_done(100, 0), buddy::bootloader::UpdateStage::PreparingUpdate);

    // update the bootloader in FLASH
    int last_reported_percent_done = -1;
    copy_bootloader_to_flash(bootloader_bin.get(), [&](int percent_done) {
        if (percent_done != last_reported_percent_done) {
            progress(calc_percent_done(100, percent_done), buddy::bootloader::UpdateStage::Updating);
            last_reported_percent_done = percent_done;
        }
    });
}
