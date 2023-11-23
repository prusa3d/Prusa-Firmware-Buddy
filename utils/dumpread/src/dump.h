// dump.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include "mapfile.h"

#define DUMP_RAM_ADDR    0x20000000
#define DUMP_RAM_SIZE    0x00020000
#define DUMP_CCMRAM_ADDR 0x10000000
#define DUMP_CCMRAM_SIZE 0x00010000
#define DUMP_OTP_ADDR    0x1fff0000
#define DUMP_OTP_SIZE    0x00008000
#define DUMP_FLASH_ADDR  0x08000000
#define DUMP_FLASH_SIZE  0x00100000

#define DUMP_REGS_GEN 0x1000ff00
#define DUMP_REGS_SCB 0x1000ff60
#define DUMP_INFO     0x1000fff0

typedef struct _dump_regs_gen_t {
    union {
        uint32_t R[16];
        struct
        {
            uint32_t R0;
            uint32_t R1;
            uint32_t R2;
            uint32_t R3;
            uint32_t R4;
            uint32_t R5;
            uint32_t R6;
            uint32_t R7;
            uint32_t R8;
            uint32_t R9;
            uint32_t R10;
            uint32_t R11;
            uint32_t R12;
            union {
                uint32_t R13;
                uint32_t SP;
            };
            union {
                uint32_t R14;
                uint32_t LR;
            };
            union {
                uint32_t R15;
                uint32_t PC;
            };
        };
    };
    uint32_t PSR;
    uint32_t PRIMASK;
    uint32_t BASEPRI;
    uint32_t FAULTMASK;
    uint32_t CONTROL;
    uint32_t MSP;
    uint32_t PSP;
    uint32_t LREXC;
} dump_regs_gen_t;

typedef struct _dump_info_t {
    uint8_t type_flags;
    uint8_t reserved[15];
} dump_info_t;

typedef struct _dump_mallinfo_t {
    uint32_t arena; /* total space allocated from system */
    uint32_t ordblks; /* number of non-inuse chunks */
    uint32_t smblks; /* unused -- always zero */
    uint32_t hblks; /* number of mmapped regions */
    uint32_t hblkhd; /* total space in mmapped regions */
    uint32_t usmblks; /* unused -- always zero */
    uint32_t fsmblks; /* unused -- always zero */
    uint32_t uordblks; /* total allocated space */
    uint32_t fordblks; /* total non-inuse space */
    uint32_t keepcost; /* top-most, releasable (via malloc_trim) space */
} dump_mallinfo_t;

typedef struct _dump_t {
    uint8_t *ram;
    uint8_t *ccmram;
    uint8_t *otp;
    uint8_t *flash;
    dump_regs_gen_t *regs_gen;
    uint32_t *regs_scb;
    dump_info_t *info;
} dump_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void dump_free(dump_t *pd);

extern dump_t *dump_load(const char *fn);

extern int dump_load_all_sections(dump_t *pd, const char *dir);

extern int dump_save_all_sections(dump_t *pd, const char *dir);

extern uint8_t *dump_get_data_ptr(dump_t *pd, uint32_t addr);

extern void dump_get_data(dump_t *pd, uint32_t addr, uint32_t size, uint8_t *data);

extern uint32_t dump_get_ui32(dump_t *pd, uint32_t addr);

extern int dump_load_bin_from_file(void *data, int size, const char *fn);

extern int dump_save_bin_to_file(void *data, int size, const char *fn);

extern uint32_t dump_find_in_flash(dump_t *pd, uint8_t *pdata, uint16_t size, uint32_t start_addr, uint32_t end_addr);

extern uint32_t dump_find_in_ram(dump_t *pd, uint8_t *pdata, uint16_t size, uint32_t start_addr, uint32_t end_addr);

extern int dump_add_symbol(uint32_t addr, uint32_t size, const char *name);

extern int dump_find_symbol_by_addr(uint32_t addr, char *name, uint32_t *offs);

extern mapfile_mem_entry_t *dump_print_var(dump_t *pd, mapfile_t *pm, const char *name);

extern mapfile_mem_entry_t *dump_print_var_ui32(dump_t *pd, mapfile_t *pm, const char *name);

extern mapfile_mem_entry_t *dump_print_var_pchar(dump_t *pd, mapfile_t *pm, const char *name);

extern void dump_print_stack(dump_t *pd, mapfile_t *pm, uint32_t addr, uint32_t depth);

extern void dump_print_all(dump_t *pd, mapfile_t *pm);

#ifdef __cplusplus
}
#endif //__cplusplus
