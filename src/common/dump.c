// dump.c

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "dump.h"
#include "w25x.h"

static const uint32_t DUMP_OFFSET = 0x00;
static const uint16_t DUMP_PAGE_SIZE = 0x100;
static const uint16_t DUMP_BUFF_SIZE = 0x100;

static const uint32_t DUMP_XFLASH_SIZE = DUMP_RAM_SIZE + DUMP_CCRAM_SIZE;

#define _STR(arg)  #arg
#define __STR(arg) _STR(arg)

static inline void dump_regs_SCB(void) {
    //copy entire SCB to CCRAM
    memcpy((uint8_t *)DUMP_REGS_SCB_ADDR, SCB, DUMP_REGS_SCB_SIZE);
}

void dump_to_xflash(void) {
    _Static_assert(sizeof(dumpinfo_t) == 16, "invalid sizeof(dumpinfo_t)");
    uint32_t addr;
    if (!dump_in_xflash_is_empty()) {
        if (dump_in_xflash_is_saved()) {
            if (!dump_in_xflash_is_displayed()) {
                return;
            }
        }
    }
    dump_regs_SCB();
    if (w25x_init()) {
        for (addr = 0; addr < DUMP_XFLASH_SIZE; addr += 0x10000) {
            w25x_block64_erase(DUMP_OFFSET + addr);
        }
        for (addr = 0; addr < DUMP_RAM_SIZE; addr += DUMP_PAGE_SIZE) {
            w25x_page_program(DUMP_OFFSET + addr, (uint8_t *)(DUMP_RAM_ADDR + addr), DUMP_PAGE_SIZE);
        }
        for (addr = 0; addr < DUMP_CCRAM_SIZE; addr += DUMP_PAGE_SIZE) {
            w25x_page_program(DUMP_OFFSET + DUMP_RAM_SIZE + addr, (uint8_t *)(DUMP_CCRAM_ADDR + addr), DUMP_PAGE_SIZE);
        }
        w25x_wait_busy();
    }
}

int dump_in_xflash_is_empty(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return (dumpinfo.type_flags & DUMP_UNDEFINED) ? 0 : 1;
}

int dump_in_xflash_is_valid(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    unsigned char dump_type = dumpinfo.type_flags & ~(DUMP_NOT_SAVED | DUMP_NOT_DISPL);
    return ((dump_type == DUMP_HARDFAULT) || (dump_type == DUMP_IWDGW) || (dump_type == DUMP_TEMPERROR));
}

int dump_in_xflash_is_saved(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return (dumpinfo.type_flags & DUMP_NOT_SAVED) ? 0 : 1;
}

int dump_in_xflash_is_displayed(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return (dumpinfo.type_flags & DUMP_NOT_DISPL) ? 0 : 1;
}

int dump_in_xflash_get_type(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return (dumpinfo.type_flags & ~(DUMP_NOT_SAVED | DUMP_NOT_DISPL));
}

unsigned short dump_in_xflash_get_code(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return dumpinfo.code;
}

void dump_in_xflash_clear_flag(uint8_t flag) {
    unsigned char dumpinfo_type;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, &dumpinfo_type, 1);
    if (dumpinfo_type & flag) {
        dumpinfo_type &= ~flag;
        w25x_wait_busy();
        w25x_enable_wr();
        w25x_page_program(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo_type), 1);
        w25x_wait_busy();
        w25x_disable_wr();
    }
}

void dump_in_xflash_set_saved(void) {
    dump_in_xflash_clear_flag(DUMP_NOT_SAVED);
}

void dump_in_xflash_set_displayed(void) {
    dump_in_xflash_clear_flag(DUMP_NOT_DISPL);
}

unsigned int dump_in_xflash_read_RAM(void *pRAM, unsigned int addr, unsigned int size) {
    if ((addr >= DUMP_RAM_ADDR) && (addr < (DUMP_RAM_ADDR + DUMP_RAM_SIZE))) {
        if (size > (DUMP_RAM_ADDR + DUMP_RAM_SIZE - addr))
            size = (DUMP_RAM_ADDR + DUMP_RAM_SIZE - addr);
        w25x_rd_data(DUMP_OFFSET + addr - DUMP_RAM_ADDR, (uint8_t *)(pRAM), size);
        return size;
    }
    return 0;
}

unsigned int dump_in_xflash_read_regs_SCB(void *pRegsSCB, unsigned int size) {
    if (size > DUMP_REGS_SCB_SIZE)
        size = DUMP_REGS_SCB_SIZE;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_REGS_SCB_ADDR - DUMP_CCRAM_ADDR, (uint8_t *)(pRegsSCB), size);
    return size;
}

unsigned int dump_in_xflash_read_regs_GEN(void *pRegsGEN, unsigned int size) {
    if (size > DUMP_REGS_GEN_SIZE)
        size = DUMP_REGS_GEN_SIZE;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_REGS_GEN_ADDR - DUMP_CCRAM_ADDR, (uint8_t *)(pRegsGEN), size);
    return size;
}

void dump_in_xflash_reset(void) {
    if (w25x_init()) {
        for (uint32_t addr = 0; addr < DUMP_XFLASH_SIZE; addr += 0x10000) {
            w25x_wait_busy();
            w25x_enable_wr();
            w25x_block64_erase(DUMP_OFFSET + addr);
        }
        w25x_wait_busy();
        w25x_disable_wr();
    }
}

void dump_in_xflash_delete(void) {
    if (w25x_init()) {
        for (uint32_t addr = 0; addr < 0x800000; addr += 0x10000) {
            w25x_wait_busy();
            w25x_enable_wr();
            w25x_block64_erase(DUMP_OFFSET + addr);
        }
        w25x_wait_busy();
        w25x_disable_wr();
    }
}

int dump_save_to_usb(const char *fn) {
    int fd;
    uint32_t addr;
    uint8_t buff[DUMP_BUFF_SIZE];
    int bw;
    int bw_total = 0;
    if (w25x_init()) {
        fd = open(fn, O_WRONLY | O_TRUNC);
        if (fd >= 0) {
            //save dumped RAM and CCRAM from xflash
            for (addr = 0; addr < DUMP_XFLASH_SIZE; addr += DUMP_BUFF_SIZE) {
                memset(buff, 0, DUMP_BUFF_SIZE);
                w25x_rd_data(addr, buff, DUMP_BUFF_SIZE);
                bw = write(fd, buff, DUMP_BUFF_SIZE);
                if (bw <= 0) {
                    break;
                }
                bw_total += bw;
            }
            //save OTP
            for (addr = 0; addr < DUMP_OTP_SIZE; addr += DUMP_BUFF_SIZE) {
                bw = write(fd, (uint8_t *)(DUMP_OTP_ADDR + addr), DUMP_BUFF_SIZE);
                if (bw <= 0) {
                    break;
                }
                bw_total += bw;
            }
            //save FLASH
            for (addr = 0; addr < DUMP_FLASH_SIZE; addr += DUMP_BUFF_SIZE) {
                bw = write(fd, (uint8_t *)(DUMP_FLASH_ADDR + addr), DUMP_BUFF_SIZE);
                if (bw <= 0) {
                    break;
                }
                bw_total += bw;
            }
            close(fd);
            if (bw_total != (DUMP_XFLASH_SIZE + DUMP_OTP_SIZE + DUMP_FLASH_SIZE)) {
                return 0;
            }
            dump_in_xflash_set_saved();
            return 1;
        }
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
