// dump.c

#include <string.h>
#include <crash_dump/dump.hpp>
#include <stdio.h>
#include "disable_interrupts.h"
#include "utility_extensions.hpp"
#include "w25x.h"
#include "FreeRTOS.h"
#include "task.h"

namespace crash_dump {

inline constexpr uint32_t dump_offset = w25x_dump_start_address;
inline constexpr uint16_t dump_buff_size = 0x100;
inline constexpr uint32_t dump_xflash_size = DUMP_RAM_SIZE + DUMP_CCRAM_SIZE;

static_assert(dump_xflash_size <= w25x_error_start_adress, "Dump overflows reserved space.");
static_assert(sizeof(dumpmessage_t) <= (w25x_pp_start_address - w25x_error_start_adress), "Error message overflows reserved space.");

static const dumpmessage_t *dumpmessage_flash = reinterpret_cast<dumpmessage_t *>(w25x_error_start_adress);

#define _STR(arg)  #arg
#define __STR(arg) _STR(arg)

static inline void dump_regs_SCB(void) {
    //copy entire SCB to CCRAM
    memcpy((uint8_t *)DUMP_REGS_SCB_ADDR, SCB, DUMP_REGS_SCB_SIZE);
}

void dump_to_xflash(void) {
    buddy::DisableInterrupts disable_interrupts;
    vTaskEndScheduler();
    if (!w25x_init()) {
        return;
    }

    _Static_assert(sizeof(dumpinfo_t) == 16, "invalid sizeof(dumpinfo_t)");
    if (dump_in_xflash_is_valid()) {
        if (!dump_in_xflash_is_displayed()) {
            return;
        }
    }
    dump_regs_SCB();
    for (uint32_t addr = 0; addr < dump_xflash_size; addr += 0x10000) {
        w25x_block64_erase(dump_offset + addr);
    }
    w25x_program(dump_offset, (uint8_t *)(DUMP_RAM_ADDR), DUMP_RAM_SIZE);
    w25x_program(dump_offset + DUMP_RAM_SIZE, (uint8_t *)(DUMP_CCRAM_ADDR), DUMP_CCRAM_SIZE);
    w25x_fetch_error();
}

/**
 * @retval true is valid
 * @retval false not valid or unable to read
 */
bool dump_in_xflash_is_valid(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo), DUMP_INFO_SIZE);
    const DumpType dump_type = dumpinfo.type_flags & ~(DumpType::DUMP_NOT_SAVED | DumpType::DUMP_NOT_DISPL);
    if (w25x_fetch_error())
        return false;
    return (dump_type == DumpType::DUMP_HARDFAULT) || (dump_type == DumpType::DUMP_IWDGW) || (dump_type == DumpType::DUMP_FATALERROR);
}

/**
 * @retval true is displayed
 * @retval false not displayed or unable to read
 */
bool dump_in_xflash_is_displayed(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    if (w25x_fetch_error())
        return false;
    return !any(dumpinfo.type_flags & DumpType::DUMP_NOT_DISPL);
}

/**
 * @retval DUMP_UNDEFINED Either there is no dump or failed to read.
 */
DumpType dump_in_xflash_get_type(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    if (w25x_fetch_error())
        dumpinfo.type_flags = DumpType::DUMP_UNDEFINED;
    return (dumpinfo.type_flags & ~(DumpType::DUMP_NOT_SAVED | DumpType::DUMP_NOT_DISPL));
}

/**
 * @todo Programming single byte more times is undocumented feature of w25x
 */
void dump_in_xflash_clear_flag(const DumpType flag) {
    DumpType dumpinfo_type;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo_type), 1);
    if (!w25x_fetch_error() && any(dumpinfo_type & flag)) {
        dumpinfo_type = dumpinfo_type & ~flag;
        w25x_program(dump_offset + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, reinterpret_cast<uint8_t *>(&dumpinfo_type), 1);
        w25x_fetch_error();
    }
}

void dump_in_xflash_set_saved(void) {
    dump_in_xflash_clear_flag(DumpType::DUMP_NOT_SAVED);
}

void dump_in_xflash_set_displayed(void) {
    dump_in_xflash_clear_flag(DumpType::DUMP_NOT_DISPL);
}

unsigned int dump_in_xflash_read_RAM(void *pRAM, unsigned int addr, unsigned int size) {
    if ((addr >= DUMP_RAM_ADDR) && (addr < (DUMP_RAM_ADDR + DUMP_RAM_SIZE))) {
        if (size > (DUMP_RAM_ADDR + DUMP_RAM_SIZE - addr))
            size = (DUMP_RAM_ADDR + DUMP_RAM_SIZE - addr);
        w25x_rd_data(dump_offset + addr - DUMP_RAM_ADDR, (uint8_t *)(pRAM), size);
        return size;
    }
    return 0;
}

unsigned int dump_in_xflash_read_regs_SCB(void *pRegsSCB, unsigned int size) {
    if (size > DUMP_REGS_SCB_SIZE)
        size = DUMP_REGS_SCB_SIZE;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_REGS_SCB_ADDR - DUMP_CCRAM_ADDR, (uint8_t *)(pRegsSCB), size);
    if (w25x_fetch_error())
        return 0;
    return size;
}

unsigned int dump_in_xflash_read_regs_GEN(void *pRegsGEN, unsigned int size) {
    if (size > DUMP_REGS_GEN_SIZE)
        size = DUMP_REGS_GEN_SIZE;
    w25x_rd_data(dump_offset + DUMP_RAM_SIZE + DUMP_REGS_GEN_ADDR - DUMP_CCRAM_ADDR, (uint8_t *)(pRegsGEN), size);
    if (w25x_fetch_error())
        return 0;
    return size;
}

void dump_in_xflash_reset(void) {
    static_assert(dump_xflash_size % w25x_block64_size == 0, "More than reserved area is erased.");
    for (uint32_t addr = 0; addr < dump_xflash_size; addr += w25x_block64_size) {
        w25x_block64_erase(dump_offset + addr);
    }
    w25x_fetch_error();
}

int dump_save_to_usb(const char *fn) {
    FILE *fd;
    uint32_t addr;
    uint8_t buff[dump_buff_size];
    int bw;
    uint32_t bw_total = 0;
    fd = fopen(fn, "w");
    if (fd != NULL) {
        //save dumped RAM and CCRAM from xflash
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
        //save OTP
        for (addr = 0; addr < DUMP_OTP_SIZE; addr += dump_buff_size) {
            bw = fwrite((void *)(DUMP_OTP_ADDR + addr), 1, dump_buff_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
        }
        //save FLASH
        for (addr = 0; addr < DUMP_FLASH_SIZE; addr += dump_buff_size) {
            bw = fwrite((void *)(DUMP_FLASH_ADDR + addr), 1, dump_buff_size, fd);
            if (bw <= 0) {
                break;
            }
            bw_total += bw;
        }
        fclose(fd);
        if (bw_total != (dump_xflash_size + DUMP_OTP_SIZE + DUMP_FLASH_SIZE)) {
            return 0;
        }
        dump_in_xflash_set_saved();
        return 1;
    }
    return 0;
}

//
void dump_hardfault_test_0(void) {
#define test_0_var (*((volatile unsigned long *)(0x4b000000)))
    test_0_var = 0;
}

//integer div by zero test
int dump_hardfault_test_1(void) {
    volatile int b = 0;
    volatile int c = 1 / b;
    return c;
}

// Dumping error message

void dump_err_to_xflash(uint16_t error_code, const char *error, const char *title) {
    buddy::DisableInterrupts disable_interrupts;
    vTaskEndScheduler();
    if (!w25x_init()) {
        return;
    }
    w25x_sector_erase(w25x_error_start_adress);

    decltype(dumpmessage_t::invalid) invalid = 0;
    const size_t title_len = strnlen(title, sizeof(dumpmessage_t::title));
    const size_t msg_len = strnlen(error, sizeof(dumpmessage_t::msg));

    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->invalid), reinterpret_cast<uint8_t *>(&invalid), sizeof(invalid));
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->error_code), reinterpret_cast<uint8_t *>(&error_code), sizeof(error_code));
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->title), reinterpret_cast<const uint8_t *>(title), title_len);
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->msg), reinterpret_cast<const uint8_t *>(error), msg_len);
    w25x_fetch_error();
}

bool dump_err_in_xflash_is_valid() {
    uint8_t invalid;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->invalid), &invalid, sizeof(dumpmessage_t::invalid));
    if (w25x_fetch_error())
        return false; // Behave as invalid message
    return !invalid;
}

bool dump_err_in_xflash_is_displayed() {
    uint8_t not_displayed;
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->not_displayed), &not_displayed, sizeof(dumpmessage_t::not_displayed));
    if (w25x_fetch_error())
        return false;
    return !not_displayed;
}

void dump_err_in_xflash_set_displayed(void) {
    uint8_t not_displayed = 0;
    w25x_program(reinterpret_cast<uint32_t>(&dumpmessage_flash->not_displayed), &not_displayed, sizeof(dumpmessage_t::not_displayed));
    w25x_fetch_error();
}

bool dump_err_in_xflash_get_message(char *msg_dst, uint16_t msg_dst_size, char *tit_dst, uint16_t tit_dst_size) {
    const size_t title_max_size = sizeof(dumpmessage_t::title) > tit_dst_size ? tit_dst_size : sizeof(dumpmessage_t::title);
    const size_t msg_max_size = sizeof(dumpmessage_t::msg) > msg_dst_size ? msg_dst_size : sizeof(dumpmessage_t::msg);

    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->title), (uint8_t *)(tit_dst), title_max_size);
    w25x_rd_data(reinterpret_cast<uint32_t>(&dumpmessage_flash->msg), (uint8_t *)(msg_dst), msg_max_size);

    if (title_max_size)
        tit_dst[title_max_size - 1] = '\0';
    if (msg_max_size)
        msg_dst[msg_max_size - 1] = '\0';

    if (w25x_fetch_error())
        return false;
    return true;
}

uint16_t dump_err_in_xflash_get_error_code(void) {
    uint16_t error_code;
    w25x_rd_data(
        reinterpret_cast<uint32_t>(&dumpmessage_flash->error_code),
        reinterpret_cast<uint8_t *>(&error_code),
        sizeof(dumpmessage_t::error_code));
    if (w25x_fetch_error())
        return 0;
    return error_code;
}

}
