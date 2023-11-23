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
#include <array>

namespace crash_dump {

typedef struct __attribute__((packed)) _info_t {
    DumpType type_flags; //
    unsigned char reserved[15]; // TODO: RTC time, code
} info_t;

typedef struct __attribute__((packed)) _message_t {
    uint8_t not_displayed; ///< not displayed == 0xFF, displayed == 0x00
    MsgType type; ///< Mark if this structure has valid data
    ErrCode error_code; ///< error_code (0 == unknown error code -> we read dumped message
    char title[MSG_TITLE_MAX_LEN];
    char msg[MSG_MAX_LEN];
} message_t;

inline constexpr uint32_t dump_offset = w25x_dump_start_address;
inline constexpr uint16_t dump_buff_size = 0x100;
inline constexpr uint32_t dump_xflash_size = RAM_SIZE + CCMRAM_SIZE;

static_assert(dump_xflash_size <= w25x_error_start_adress, "Dump overflows reserved space.");
static_assert(sizeof(message_t) <= (w25x_pp_start_address - w25x_error_start_adress), "Error message overflows reserved space.");

static const message_t *dumpmessage_flash = reinterpret_cast<message_t *>(w25x_error_start_adress);

static inline void dump_regs_SCB() {
    // copy entire SCB to CCMRAM
    memcpy((uint8_t *)REGS_SCB_ADDR, SCB, REGS_SCB_SIZE);
}

void save_dump() {
    buddy::DisableInterrupts disable_interrupts;
    vTaskEndScheduler();
    if (!w25x_init()) {
        return;
    }

    _Static_assert(sizeof(info_t) == 16, "invalid sizeof(dumpinfo_t)");
    if (dump_is_valid()) {
        if (!dump_is_displayed()) {
            return;
        }
    }
    dump_regs_SCB();
    for (uint32_t addr = 0; addr < dump_xflash_size; addr += 0x10000) {
        w25x_block64_erase(dump_offset + addr);
    }
    w25x_program(dump_offset, (uint8_t *)(RAM_ADDR), RAM_SIZE);
    w25x_program(dump_offset + RAM_SIZE, (uint8_t *)(CCMRAM_ADDR), CCMRAM_SIZE);
    w25x_fetch_error();
}

bool dump_is_valid() {
    info_t dumpinfo;
    w25x_rd_data(dump_offset + RAM_SIZE + CCMRAM_SIZE - INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo), INFO_SIZE);
    if (w25x_fetch_error())
        return false;

    const uint8_t dump_type = ftrstd::to_underlying(dumpinfo.type_flags & DumpType::TYPEMASK);
    return dump_type && ((dump_type & (dump_type - 1)) == 0); // Is exactly one bit set?
}

bool dump_is_displayed() {
    info_t dumpinfo;
    w25x_rd_data(dump_offset + RAM_SIZE + CCMRAM_SIZE - INFO_SIZE, (uint8_t *)(&dumpinfo), INFO_SIZE);
    if (w25x_fetch_error())
        return false;
    return !any(dumpinfo.type_flags & DumpType::NOT_DISPL);
}

DumpType dump_get_type() {
    info_t dumpinfo;
    w25x_rd_data(dump_offset + RAM_SIZE + CCMRAM_SIZE - INFO_SIZE, (uint8_t *)(&dumpinfo), INFO_SIZE);
    if (w25x_fetch_error())
        dumpinfo.type_flags = DumpType::UNDEFINED;
    return (dumpinfo.type_flags & ~(DumpType::NOT_SAVED | DumpType::NOT_DISPL));
}

/**
 * @todo Programming single byte more times is undocumented feature of w25x
 */
static void dump_clear_flag(const DumpType flag) {
    DumpType dumpinfo_type;
    w25x_rd_data(dump_offset + RAM_SIZE + CCMRAM_SIZE - INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo_type), 1);
    if (!w25x_fetch_error() && any(dumpinfo_type & flag)) {
        dumpinfo_type = dumpinfo_type & ~flag;
        w25x_program(dump_offset + RAM_SIZE + CCMRAM_SIZE - INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo_type), 1);
        w25x_fetch_error();
    }
}

/**
 * @brief Mark dump as saved.
 */
static void dump_set_saved() {
    dump_clear_flag(DumpType::NOT_SAVED);
}

void dump_set_displayed() {
    dump_clear_flag(DumpType::NOT_DISPL);
}

size_t load_dump_RAM(void *pRAM, uint32_t addr, size_t size) {
    if ((addr >= RAM_ADDR) && (addr < (RAM_ADDR + RAM_SIZE))) {
        if (size > (RAM_ADDR + RAM_SIZE - addr))
            size = (RAM_ADDR + RAM_SIZE - addr);
        w25x_rd_data(dump_offset + addr - RAM_ADDR, (uint8_t *)(pRAM), size);
        return size;
    } else if ((addr >= CCMRAM_ADDR) && (addr < (CCMRAM_ADDR + CCMRAM_SIZE))) {
        if (size > (CCMRAM_ADDR + CCMRAM_SIZE - addr))
            size = (CCMRAM_ADDR + CCMRAM_SIZE - addr);
        w25x_rd_data(dump_offset + RAM_SIZE + addr - CCMRAM_ADDR, (uint8_t *)(pRAM), size);
        return size;
    }
    return 0;
}

size_t load_dump_regs_SCB(void *pRegsSCB, size_t size) {
    if (size > REGS_SCB_SIZE)
        size = REGS_SCB_SIZE;
    w25x_rd_data(dump_offset + RAM_SIZE + REGS_SCB_ADDR - CCMRAM_ADDR, (uint8_t *)(pRegsSCB), size);
    if (w25x_fetch_error())
        return 0;
    return size;
}

size_t load_dump_regs_GEN(void *pRegsGEN, size_t size) {
    if (size > REGS_GEN_SIZE)
        size = REGS_GEN_SIZE;
    w25x_rd_data(dump_offset + RAM_SIZE + REGS_GEN_ADDR - CCMRAM_ADDR, (uint8_t *)(pRegsGEN), size);
    if (w25x_fetch_error())
        return 0;
    return size;
}

void dump_reset() {
    static_assert(dump_xflash_size % w25x_block64_size == 0, "More than reserved area is erased.");
    for (uint32_t addr = 0; addr < dump_xflash_size; addr += w25x_block64_size) {
        w25x_block64_erase(dump_offset + addr);
    }
    w25x_fetch_error();
}

bool save_dump_to_usb(const char *fn) {
    FILE *fd;
    uint32_t addr;
    uint8_t buff[dump_buff_size];
    int bw;
    uint32_t bw_total = 0;
    fd = fopen(fn, "w");
    if (fd != NULL) {
        // save dumped RAM and CCMRAM from xflash
        for (addr = 0; addr < dump_xflash_size; addr += dump_buff_size) {
            memset(buff, 0, dump_buff_size);
            w25x_rd_data(addr, buff, dump_buff_size);
            if (w25x_fetch_error()) {
                break;
            }
            bw = fwrite(buff, 1, dump_buff_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
        }
        // save OTP
        for (addr = 0; addr < OTP_SIZE; addr += dump_buff_size) {
            bw = fwrite((void *)(OTP_ADDR + addr), 1, dump_buff_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
        }
        // save FLASH
        for (addr = 0; addr < FLASH_SIZE; addr += dump_buff_size) {
            bw = fwrite((void *)(FLASH_ADDR + addr), 1, dump_buff_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
        }
        fclose(fd);
        if (bw_total != (dump_xflash_size + OTP_SIZE + FLASH_SIZE)) {
            return false;
        }
        dump_set_saved();
        return true;
    }
    return false;
}

void save_message(MsgType type, uint16_t error_code, const char *error, const char *title) {
    static_assert(ftrstd::to_underlying(ErrCode::ERR_UNDEF) == 0, "This uses 0 as undefined error");

    buddy::DisableInterrupts disable_interrupts;
    vTaskEndScheduler();
    if (!w25x_init()) {
        return;
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
    w25x_fetch_error();
}

MsgType message_get_type() {
    uint8_t type;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->type), &type, sizeof(message_t::type));
    if (w25x_fetch_error())
        return MsgType::EMPTY; // Behave as invalid message
    return MsgType(type);
}

bool message_is_displayed() {
    uint8_t not_displayed;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->not_displayed), &not_displayed, sizeof(message_t::not_displayed));
    if (w25x_fetch_error())
        return false;
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

    if (title_max_size)
        tit_dst[title_max_size - 1] = '\0';
    if (msg_max_size)
        msg_dst[msg_max_size - 1] = '\0';

    if (w25x_fetch_error())
        return false;
    return true;
}

uint16_t load_message_error_code() {
    uint16_t error_code;
    w25x_rd_data(
        reinterpret_cast<uint32_t>(&dumpmessage_flash->error_code),
        reinterpret_cast<uint8_t *>(&error_code),
        sizeof(message_t::error_code));
    if (w25x_fetch_error())
        return ftrstd::to_underlying(ErrCode::ERR_UNDEF);
    return error_code;
}

} // namespace crash_dump
