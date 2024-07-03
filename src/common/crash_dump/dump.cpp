// dump.c

#include <string.h>
#include <crash_dump/dump.hpp>
#include <stdio.h>
#include "disable_interrupts.h"
#include "utility_extensions.hpp"
#include "w25x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <error_codes.hpp>
#include "safe_state.h"
#include <wdt.hpp>
#include <algorithm>
extern "C" {
#include "CrashCatcher.h"
}

#include <version.h>

namespace crash_dump {

/// While dumping, this stores size of already dumped data
static uint32_t dump_size;
static bool dump_breakpoint_paused = false;
static bool wdg_reset_safeguard = false; ///< Safeguard to prevent multiple refresh of watchdog in dump

/// Just random value, used to check that dump is probably valid in flash
inline constexpr uint32_t CRASH_DUMP_MAGIC_NR = 0x3DC53F;

/// Just random value, used to check that message is probably valid in flash
inline constexpr uint32_t MESSAGE_DUMP_MAGIC_NR = 0xD87FA0DF;
typedef struct __attribute__((packed)) {
    /// Magic number, that indicates that crash dump is valid
    uint32_t crash_dump_magic_nr;
    DumpFlags dump_flags;
    uint32_t dump_size;
} info_t;

typedef struct __attribute__((packed)) {
    uint32_t message_magic_nr; ///< Magic number that indicates that message in flash is valid
    uint8_t not_displayed; ///< not displayed == 0xFF, displayed == 0x00
    MsgType type; ///< Mark if this structure has valid data
    ErrCode error_code; ///< error_code (0 == unknown error code -> we read dumped message
    char title[MSG_TITLE_MAX_LEN];
    char msg[MSG_MAX_LEN];
} message_t;

/// Position of dump header
inline constexpr uint32_t dump_header_addr = w25x_dump_start_address;
/// Position of dump data
inline constexpr uint32_t dump_data_addr = dump_header_addr + sizeof(info_t);
/// Max size of dump (header + data)
inline constexpr uint32_t dump_max_size = w25x_dump_size;
/// Max size of dump data
inline constexpr uint32_t dump_max_data_size = dump_max_size - sizeof(info_t);

static_assert(sizeof(message_t) <= (w25x_pp_start_address - w25x_error_start_adress), "Error message overflows reserved space.");
/// Position of dump message in flash
static const message_t *dumpmessage_flash = reinterpret_cast<message_t *>(w25x_error_start_adress);

enum {
#if PRINTER_IS_PRUSA_MINI
    // dumped ram area (128kb)
    RAM_ADDR = 0x20000000,
    RAM_SIZE = 0x00020000,
#elif (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX)
    // dumped ram area (192kb)
    RAM_ADDR = 0x20000000,
    RAM_SIZE = 0x00030000,
#else
    #error "Unknown PRINTER_TYPE!"
#endif

    // dumped ccmram area (64kb)
    CCMRAM_ADDR = CCMDATARAM_BASE,
    CCMRAM_SIZE = CCMDATARAM_END - CCMDATARAM_BASE,

    SCB_ADDR = (uintptr_t)SCB_BASE,
    SCB_SIZE = sizeof(SCB_Type),

};

static bool dump_read_header(info_t &dumpinfo) {
    w25x_rd_data(dump_header_addr, (uint8_t *)(&dumpinfo), sizeof(dumpinfo));
    if (w25x_fetch_error()) {
        return false;
    }
    return true;
}

bool dump_is_exported() {
    info_t dumpinfo;
    dump_read_header(dumpinfo);
    return !any(dumpinfo.dump_flags & DumpFlags::EXPORTED);
}

bool dump_is_valid() {
    info_t dumpinfo;
    dump_read_header(dumpinfo);
    return (bool)(dumpinfo.crash_dump_magic_nr == CRASH_DUMP_MAGIC_NR) && dumpinfo.dump_size > 0 && dumpinfo.dump_size <= dump_max_data_size;
}

bool dump_is_displayed() {
    info_t dumpinfo;
    dump_read_header(dumpinfo);
    return !any(dumpinfo.dump_flags & DumpFlags::DISPL);
}

size_t dump_get_size() {
    if (!dump_is_valid()) {
        return 0;
    }
    info_t dumpinfo;
    dump_read_header(dumpinfo);
    return dumpinfo.dump_size;
}

bool dump_read_data(size_t offset, size_t size, uint8_t *ptr) {
    w25x_rd_data(dump_data_addr + offset, ptr, size);
    return !w25x_fetch_error();
}

/**
 * @todo Programming single byte more times is undocumented feature of w25x
 */
static void dump_set_flag(const DumpFlags flag) {
    DumpFlags dump_flags;
    w25x_rd_data(dump_header_addr + offsetof(info_t, dump_flags), reinterpret_cast<uint8_t *>(&dump_flags), 1);
    // set bit to zero - that is active state of this bit
    if (!w25x_fetch_error() && any(dump_flags & flag)) {
        dump_flags = dump_flags & ~flag;
        w25x_program(dump_header_addr + offsetof(info_t, dump_flags), reinterpret_cast<uint8_t *>(&dump_flags), 1);
        w25x_fetch_error();
    }
}

void dump_set_exported() {
    dump_set_flag(DumpFlags::EXPORTED);
}

void dump_set_displayed() {
    dump_set_flag(DumpFlags::DISPL);
}

void dump_reset() {
    static_assert(dump_header_addr % w25x_block64_size == 0 && (dump_header_addr + dump_max_size) % w25x_block_size == 0, "More than reserved area is erased.");
    uint32_t addr = dump_header_addr;
    // first fast-erase multiple sectors with 64KiB blocks
    for (; addr + w25x_block64_size <= dump_max_size; addr += w25x_block64_size) {
        w25x_block64_erase(addr);
    }
    // now erase rest of the blocks
    for (; addr + w25x_block_size <= dump_max_size; addr += w25x_block_size) {
        w25x_sector_erase(addr);
    }

    w25x_fetch_error();
}

bool save_dump_to_usb(const char *fn) {
    FILE *fd;
    constexpr uint16_t dump_buff_size = 0x100;
    uint8_t buff[dump_buff_size];
    int bw;
    uint32_t bw_total = 0;

    info_t dump_info;
    if (!dump_read_header(dump_info)) {
        return false;
    }

    fd = fopen(fn, "w");
    if (fd != NULL) {
        // save dumped RAM and CCMRAM from xflash
        for (uint32_t offset = 0; offset < dump_info.dump_size;) {
            size_t read_size = std::min(sizeof(buff), (size_t)(dump_info.dump_size - offset));

            memset(buff, 0, read_size);
            if (!dump_read_data(offset, read_size, buff)) {
                break;
            }
            bw = fwrite(buff, 1, read_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
            offset += read_size;
        }
        fclose(fd);
        if (bw_total != dump_info.dump_size) {
            return false;
        }
        dump_set_exported();
        return true;
    }
    return false;
}

void save_message(MsgType type, uint16_t error_code, const char *error, const char *title) {
    static_assert(ftrstd::to_underlying(ErrCode::ERR_UNDEF) == 0, "This uses 0 as undefined error");

    // break in case debugger is attached and avoid saving message to eeprom
    crash_dump::before_dump();

    buddy::DisableInterrupts disable_interrupts;
    vTaskEndScheduler();
    if (!w25x_init()) {
        return;
    }

    force_save_message_without_dump(type, error_code, error, title);
}

void force_save_message_without_dump(MsgType type, uint16_t error_code, const char *error, const char *title) {
    static_assert(ftrstd::to_underlying(ErrCode::ERR_UNDEF) == 0, "This uses 0 as undefined error");

    assert(error != nullptr);

    if (title == nullptr) {
        title = "";
    }

    w25x_sector_erase(w25x_error_start_adress);

    const size_t title_len = strnlen(title, std::size(dumpmessage_flash->title));
    const size_t msg_len = strnlen(error, std::size(dumpmessage_flash->msg));

    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->type), reinterpret_cast<uint8_t *>(&type), sizeof(type));
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->error_code), reinterpret_cast<uint8_t *>(&error_code), sizeof(error_code));
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->title), reinterpret_cast<const uint8_t *>(title),
        std::min(std::size(dumpmessage_flash->title), title_len + 1)); // +1 for null terminator
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->msg), reinterpret_cast<const uint8_t *>(error),
        std::min(std::size(dumpmessage_flash->msg), msg_len + 1));

    // write magic number to make flash record valid
    uint32_t magic = MESSAGE_DUMP_MAGIC_NR;
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->message_magic_nr), reinterpret_cast<const uint8_t *>(&magic), sizeof(magic));
    w25x_fetch_error();
}

bool message_is_valid() {
    uint32_t magic;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->message_magic_nr), reinterpret_cast<uint8_t *>(&magic), sizeof(message_t::message_magic_nr));
    if (w25x_fetch_error()) {
        return false;
    }
    return magic == MESSAGE_DUMP_MAGIC_NR;
}

MsgType message_get_type() {
    uint8_t type;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->type), &type, sizeof(message_t::type));
    if (w25x_fetch_error()) {
        return MsgType::EMPTY; // Behave as invalid message
    }
    return MsgType(type);
}

bool message_is_displayed() {
    uint8_t not_displayed;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->not_displayed), &not_displayed, sizeof(message_t::not_displayed));
    if (w25x_fetch_error()) {
        return false;
    }
    return !not_displayed;
}

void message_set_displayed() {
    uint8_t not_displayed = 0;
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->not_displayed), &not_displayed, sizeof(message_t::not_displayed));
    w25x_fetch_error();
}

bool load_message(char *msg_dst, size_t msg_dst_size, char *tit_dst, size_t tit_dst_size) {
    const size_t title_max_size = std::min(std::size(dumpmessage_flash->title), tit_dst_size);
    const size_t msg_max_size = std::min(std::size(dumpmessage_flash->msg), msg_dst_size);

    if (title_max_size > 0) {
        w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->title), (uint8_t *)(tit_dst), title_max_size);
    }
    if (msg_max_size > 0) {
        w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->msg), (uint8_t *)(msg_dst), msg_max_size);
    }

    if (title_max_size) {
        tit_dst[title_max_size - 1] = '\0';
    }
    if (msg_max_size) {
        msg_dst[msg_max_size - 1] = '\0';
    }

    if (w25x_fetch_error()) {
        return false;
    }
    return true;
}

uint16_t load_message_error_code() {
    uint16_t error_code;
    w25x_rd_data(
        reinterpret_cast<uint32_t>(&dumpmessage_flash->error_code),
        reinterpret_cast<uint8_t *>(&error_code),
        sizeof(message_t::error_code));
    if (w25x_fetch_error()) {
        return ftrstd::to_underlying(ErrCode::ERR_UNDEF);
    }
    return error_code;
}

static void dump_failed() {
    // nothing left to do here, when dump fails just restart
    HAL_NVIC_SystemReset();
}

static constexpr CrashCatcherMemoryRegion regions[] = {
    { crash_dump::SCB_ADDR, crash_dump::SCB_ADDR + crash_dump::SCB_SIZE, CRASH_CATCHER_WORD },
    { crash_dump::RAM_ADDR, crash_dump::RAM_ADDR + crash_dump::RAM_SIZE, CRASH_CATCHER_BYTE },
    { crash_dump::CCMRAM_ADDR, crash_dump::CCMRAM_ADDR + crash_dump::CCMRAM_SIZE, CRASH_CATCHER_BYTE },
    { reinterpret_cast<uintptr_t>(&project_build_identification), reinterpret_cast<uintptr_t>(&project_build_identification) + sizeof(project_build_identification), CRASH_CATCHER_BYTE },
    { 0xFFFFFFFF, 0, CRASH_CATCHER_BYTE },
};

void before_dump() {
    // avoid triggering before_dump multiple times (it can be called before BSOD and then again in hardfault handler)
    if (!dump_breakpoint_paused) {
        dump_breakpoint_paused = true;
        buddy_disable_heaters(); // put HW to safe state
#ifdef _DEBUG
        if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
            // if case debugger is attached, issue breakpoint instead of crash dump.
            // If you still want to do crash dump, resume the processor
            CRASH_CATCHER_BREAKPOINT();
        }
#endif
    }

    // this function is called before WDR or when preparing to dump on flash. Flash erase takes
    // 300ms typically. But according to dataheet, it can take multiple seconds, and we might need
    // to initialize it first. Refresh the watchdog once to give us an additioanl 4s to dump it.
    if (!crash_dump::wdg_reset_safeguard) {
        wdt_iwdg_refresh();

        // disable the callback itself to avoid WDR recursion and get the last full effective timeout
        wdt_iwdg_warning_cb = nullptr;
        crash_dump::wdg_reset_safeguard = true;
    }
}

void trigger_crash_dump() {
    before_dump();

    // trigger hardfault, hardfault will dump the processor state
    CRASH_CATCHER_INVALID_INSTRUCTION();

    // just to make function no-return
    while (1) {
    }
}

} // namespace crash_dump

const CrashCatcherMemoryRegion *CrashCatcher_GetMemoryRegions(void) {
    return crash_dump::regions;
}

void CrashCatcher_DumpStart([[maybe_unused]] const CrashCatcherInfo *pInfo) {
    __disable_irq();
    vTaskEndScheduler();

    crash_dump::before_dump();

    if (!w25x_init()) {
        crash_dump::dump_failed();
    }

    if (crash_dump::dump_is_valid() && !crash_dump::dump_is_displayed()) {
        // do not overwrite dump that is already valid & wasn't displayed to user yet
        crash_dump::dump_failed();
    }

    crash_dump::dump_reset();

    crash_dump::dump_size = 0;
}

void CrashCatcher_DumpMemory(const void *pvMemory, CrashCatcherElementSizes element_size, size_t elementCount) {
    if (element_size == CRASH_CATCHER_BYTE) {
        if (crash_dump::dump_size + elementCount > crash_dump::dump_max_data_size) {
            crash_dump::dump_failed();
        }

        w25x_program(crash_dump::dump_data_addr + crash_dump::dump_size, (uint8_t *)pvMemory, elementCount);
        crash_dump::dump_size += elementCount;
    } else if (element_size == CRASH_CATCHER_WORD) {
        if (crash_dump::dump_size + elementCount * sizeof(uint32_t) > crash_dump::dump_max_data_size) {
            crash_dump::dump_failed();
        }

        const uint32_t *ptr = reinterpret_cast<const uint32_t *>(pvMemory);
        while (elementCount) {
            uint32_t word = *ptr++;
            w25x_program(crash_dump::dump_data_addr + crash_dump::dump_size, (uint8_t *)ptr, sizeof(word));
            crash_dump::dump_size += sizeof(word);
            elementCount--;
        }
    } else {
        crash_dump::dump_failed();
    }

    if (w25x_fetch_error()) {
        crash_dump::dump_failed();
    }
}

CrashCatcherReturnCodes CrashCatcher_DumpEnd(void) {
    // if we got up to here with success, program dump header
    crash_dump::info_t dump_info {
        .crash_dump_magic_nr = crash_dump::CRASH_DUMP_MAGIC_NR,
        .dump_flags = crash_dump::DumpFlags::DEFAULT,
        .dump_size = crash_dump::dump_size,
    };
    w25x_program(crash_dump::dump_header_addr, (uint8_t *)&dump_info, sizeof(dump_info));
    if (w25x_fetch_error()) {
        crash_dump::dump_failed();
    }

    // All done, now restart and display BSOD
    HAL_NVIC_SystemReset();

    // need to return something, but it should never get here.
    return CRASH_CATCHER_TRY_AGAIN;
}
