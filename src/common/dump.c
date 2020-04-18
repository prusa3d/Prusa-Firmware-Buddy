// dump.c

#include "dump.h"
#include "ff.h"
#include "w25x.h"

#define DUMP_OFFSET 0x00000000

#define DUMP_PAGE_SIZE 0x100

#define DUMP_BUFF_SIZE 0x100

#define DUMP_XFLASH_SIZE (DUMP_RAM_SIZE + DUMP_CCRAM_SIZE)

#define _STR(arg)  #arg
#define __STR(arg) _STR(arg)

static inline void dump_regs_SCB(void) {
    //copy entire SCB to CCRAM
    memcpy((uint8_t *)DUMP_REGS_SCB_ADDR, SCB, DUMP_REGS_SCB_SIZE);
}

void dump_to_xflash(void) {
    uint32_t addr;
    dump_regs_SCB();
    if (w25x_init()) {
        for (addr = 0; addr < DUMP_XFLASH_SIZE; addr += 0x10000) {
            w25x_wait_busy();
            w25x_enable_wr();
            w25x_block64_erase(DUMP_OFFSET + addr);
        }
        for (addr = 0; addr < DUMP_RAM_SIZE; addr += DUMP_PAGE_SIZE) {
            w25x_wait_busy();
            w25x_enable_wr();
            w25x_page_program(DUMP_OFFSET + addr, (uint8_t *)(DUMP_RAM_ADDR + addr), DUMP_PAGE_SIZE);
        }
        for (addr = 0; addr < DUMP_CCRAM_SIZE; addr += DUMP_PAGE_SIZE) {
            w25x_wait_busy();
            w25x_enable_wr();
            w25x_page_program(DUMP_OFFSET + DUMP_RAM_SIZE + addr, (uint8_t *)(DUMP_CCRAM_ADDR + addr), DUMP_PAGE_SIZE);
        }
        w25x_wait_busy();
        w25x_disable_wr();
    }
}

int dump_in_xflash_is_valid(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    unsigned char dump_type = dumpinfo.type_flags & ~DUMP_NOT_SAVED;
    return ((dump_type == DUMP_HARDFAULT) || (dump_type == DUMP_IWDGW));
}

int dump_in_xflash_is_saved(void) {
    dumpinfo_t dumpinfo;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo), DUMP_INFO_SIZE);
    return (dumpinfo.type_flags & DUMP_NOT_SAVED) ? 0 : 1;
}

void dump_in_xflash_set_saved(void) {
    unsigned char dumpinfo_type;
    w25x_rd_data(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, &dumpinfo_type, 1);
    if (dumpinfo_type & DUMP_NOT_SAVED) {
        dumpinfo_type &= ~DUMP_NOT_SAVED;
        w25x_wait_busy();
        w25x_enable_wr();
        w25x_page_program(DUMP_OFFSET + DUMP_RAM_SIZE + DUMP_CCRAM_SIZE - DUMP_INFO_SIZE, (uint8_t *)(&dumpinfo_type), 1);
        w25x_wait_busy();
        w25x_disable_wr();
    }
}

int dump_save_to_usb(const char *fn) {
    FIL fil;
    uint32_t addr;
    uint8_t buff[DUMP_BUFF_SIZE];
    UINT bw;
    UINT bw_total = 0;
    if (w25x_init()) {
        if (f_open(&fil, fn, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            //save dumped RAM and CCRAM from xflash
            for (addr = 0; addr < DUMP_XFLASH_SIZE; addr += DUMP_BUFF_SIZE) {
                memset(buff, 0, DUMP_BUFF_SIZE);
                w25x_rd_data(addr, buff, DUMP_BUFF_SIZE);
                if (f_write(&fil, buff, DUMP_BUFF_SIZE, &bw) != FR_OK)
                    break;
                bw_total += bw;
            }
            //save OTP
            for (addr = 0; addr < DUMP_OTP_SIZE; addr += DUMP_BUFF_SIZE) {
                if (f_write(&fil, (uint8_t *)(DUMP_OTP_ADDR + addr), DUMP_BUFF_SIZE, &bw) != FR_OK)
                    break;
                bw_total += bw;
            }
            //save FLASH
            for (addr = 0; addr < DUMP_FLASH_SIZE; addr += DUMP_BUFF_SIZE) {
                if (f_write(&fil, (uint8_t *)(DUMP_FLASH_ADDR + addr), DUMP_BUFF_SIZE, &bw) != FR_OK)
                    break;
                bw_total += bw;
            }
            f_close(&fil);
            if (bw_total != (DUMP_XFLASH_SIZE + DUMP_OTP_SIZE + DUMP_FLASH_SIZE))
                return 0;
            dump_in_xflash_set_saved();
            return 1;
        }
    }
    return 0;
}

#ifdef _DEBUG

//
void dump_hardfault_test_0(void) {
    #define test_0_var (*((volatile unsigned long *)(0x4b000000)))
    test_0_var = 0;
}

//integer div by zero test
void dump_hardfault_test_1(void) {
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    c = c;
}

#endif //_DEBUG
