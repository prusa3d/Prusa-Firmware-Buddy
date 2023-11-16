

#include "stm32g0xx_hal.h"
#include <algorithm>
#include <cstring>
extern "C" {
#include "CrashCatcher.h"
}
#include "puppies/crash_dump_shared.hpp"

extern const uint32_t __dump_makers_end; // use as &__dump_makers_end to get the address of the end of the dump makers section
// Marks the start of the dumping page. __dump_makers_end should be already correctly aligned, but better be safe than sorry and align it here as well
const uint32_t dump_region_start {
    (reinterpret_cast<uint32_t>(&__dump_makers_end) % FLASH_PAGE_SIZE) == 0
        ? reinterpret_cast<uint32_t>(&__dump_makers_end)
        : reinterpret_cast<uint32_t>(&__dump_makers_end)
            - (reinterpret_cast<uint32_t>(&__dump_makers_end) % FLASH_PAGE_SIZE)
            + FLASH_PAGE_SIZE
};
static_assert(FLASH_BASE % FLASH_PAGE_SIZE == 0); // if both dump_region_start and FLASH_BASE are page alligned, sum of those two is also aligned

// actual size of dump is 36960B (0x9060), so we need little more space than 18 sectors
constexpr uint32_t DUMP_SIZE { 19 * FLASH_PAGE_SIZE };

static uint64_t wr_data_buffer;
static uint8_t wr_data_buffer_offset;
static size_t current_dump_start_offset;

static void stop();
static void write_doubleword();

const CrashCatcherMemoryRegion *CrashCatcher_GetMemoryRegions(void) {
    static const CrashCatcherMemoryRegion regions[] = {
        { 0x20000000, 0x20009000, CRASH_CATCHER_BYTE },
        { 0xFFFFFFFF, 0, CRASH_CATCHER_BYTE }

    };
    return regions;
}

extern const uint32_t __fw_descriptor_start; // use as &__fw_descriptor_start to get the address of the start of the FW_DESCRIPTOR region
extern const uint32_t __fw_descriptor_length; // use as &__fw_descriptor_length to get the length of the FW_DESCRIPTOR region
extern const uint32_t __flash_start; // use as &__flash_start to get the address of the start of the FLASH region

// Used to read the FW_DESCRIPTOR section persistent data, attribute 'used' is so that it's not optimized away. It's unitialized because we want the data already in the section
__attribute__((section(".fw_descriptor"), used)) const puppy_crash_dump::FWDescriptor fw_descriptor;

void CrashCatcher_DumpStart([[maybe_unused]] const CrashCatcherInfo *pInfo) {
    __disable_irq();

    // check if there is already some crashdump in flash, if yes, stop and dont overwrite it
    if (fw_descriptor.stored_type == puppy_crash_dump::FWDescriptor::StoredType::crash_dump) {
        stop();
    }

    if (dump_region_start + DUMP_SIZE > reinterpret_cast<uint32_t>(&__fw_descriptor_start) - reinterpret_cast<uint32_t>(&__fw_descriptor_start) % FLASH_PAGE_SIZE) {
        stop(); // Not enough space for dump to be placed
    }

    // unlock flash
    HAL_FLASH_Unlock();

    // erase all sectors that will be needed
    uint32_t PageError;
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = (dump_region_start - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages = DUMP_SIZE / FLASH_PAGE_SIZE;
    if (HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &PageError); status != HAL_OK) {
        stop();
    }

    static constexpr uint32_t last_page_idx { 63 };
    if (erase.Page + erase.NbPages - 1 < last_page_idx) { // if last page was not erased
        erase.Page = last_page_idx;
        erase.NbPages = 1;
        // erase last page as the descriptor will reside there
        if (HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &PageError); status != HAL_OK) {
            stop();
        }
    }

    wr_data_buffer_offset = 0;
    current_dump_start_offset = 0;
}

void CrashCatcher_DumpMemory(const void *pvMemory, CrashCatcherElementSizes elementSize, size_t elementCount) {
    if (elementSize != CRASH_CATCHER_BYTE) {
        stop();
    }

    while (elementCount > 0) {
        // data has to be written 8 bytes at a time. Therefore first wr_data_buffer is filled, and when its full, its written to flash.

        // write data to wr_data_buffer (1-8bytes)
        size_t write_size = std::min(sizeof(wr_data_buffer) - wr_data_buffer_offset, elementCount);
        memcpy(reinterpret_cast<uint8_t *>(&wr_data_buffer) + wr_data_buffer_offset, pvMemory, write_size);
        wr_data_buffer_offset += write_size;

        // wr_data_buffer is full, write it to flash now
        if (wr_data_buffer_offset == sizeof(wr_data_buffer)) {
            write_doubleword();
            wr_data_buffer_offset = 0;
        }

        pvMemory = reinterpret_cast<const uint8_t *>(pvMemory) + write_size;
        elementCount -= write_size;
    }
}

static void write_doubleword() {
    if (current_dump_start_offset + sizeof(wr_data_buffer) > DUMP_SIZE) {
        stop();
    }
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
        dump_region_start + current_dump_start_offset,
        wr_data_buffer);
    if (status != HAL_OK) {
        stop();
    }

    current_dump_start_offset += sizeof(wr_data_buffer);
}

CrashCatcherReturnCodes CrashCatcher_DumpEnd(void) {
    // now write descriptor, so that we can check for valid crash dump
    puppy_crash_dump::FWDescriptor modified_descriptor;
    modified_descriptor.dump_offset = dump_region_start - reinterpret_cast<uint32_t>(&__flash_start);
    modified_descriptor.dump_size = current_dump_start_offset;
    modified_descriptor.stored_type = puppy_crash_dump::FWDescriptor::StoredType::crash_dump;

    for (uint32_t i = 0; i < std::min(reinterpret_cast<uint32_t>(&__fw_descriptor_length), static_cast<uint32_t>(sizeof(puppy_crash_dump::FWDescriptor)));
         i += sizeof(wr_data_buffer)) {
        memcpy(&wr_data_buffer, reinterpret_cast<uint8_t *>(&modified_descriptor) + i, sizeof(wr_data_buffer));
        if (auto rc = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, reinterpret_cast<uint32_t>(&__fw_descriptor_start) + i, wr_data_buffer);
            rc != HAL_OK) {
            stop();
        }
    }

    stop();
    return CRASH_CATCHER_EXIT;
}

static void stop() {
    HAL_FLASH_Lock();
    while (1) {
    }
}
