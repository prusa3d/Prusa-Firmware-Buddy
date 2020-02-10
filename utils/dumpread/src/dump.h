// dump.h
#ifndef _DUMP_H
#define _DUMP_H

#include <inttypes.h>
#include <stdio.h>


#define DUMP_RAM_ADDR    0x20000000
#define DUMP_RAM_SIZE    0x00020000
#define DUMP_CCRAM_ADDR    0x10000000
#define DUMP_CCRAM_SIZE    0x00010000
#define DUMP_OTP_ADDR    0x1fff0000
#define DUMP_OTP_SIZE    0x00008000
#define DUMP_FLASH_ADDR    0x08000000
#define DUMP_FLASH_SIZE    0x00100000

#define DUMP_REGS_GEN    0x1000ff00
#define DUMP_REGS_SCB    0x1000ff60


#pragma pack(push)
#pragma pack(1)

typedef struct _dump_regs_gen_t
{
    union
    {
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
            union
            {
                uint32_t R13;
                uint32_t SP;
            };
            union
            {
                uint32_t R14;
                uint32_t LR;
            };
            union
            {
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

typedef struct _dump_tcb_t
{
    uint32_t pxTopOfStack;
    uint32_t xStateListItem[5];
    uint32_t xEventListItem[5];
    uint32_t uxPriority;
    uint32_t pxStack;
    char pcTaskName[16];
} dump_tcb_t;

#pragma pack(pop)


typedef struct _dump_t
{
    uint8_t* ram;
    uint8_t* ccram;
    uint8_t* otp;
    uint8_t* flash;
    dump_regs_gen_t* regs_gen;
    uint8_t* regs_scb;
} dump_t;



#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void dump_free(dump_t* pd);


extern dump_t* dump_load(const char* fn);

extern int dump_load_all_sections(dump_t* pd, const char* dir);

extern int dump_save_all_sections(dump_t* pd, const char* dir);


extern uint8_t* dump_get_data_ptr(dump_t* pd, uint32_t addr);

extern void dump_get_data(dump_t* pd, uint32_t addr, uint32_t size, uint8_t* data);

extern uint32_t dump_get_ui32(dump_t* pd, uint32_t addr);


extern void dump_print_hardfault_simple(dump_t* pd);

extern void dump_print_hardfault_detail(dump_t* pd);


extern int dump_load_bin_from_file(void* data, int size, const char* fn);

extern int dump_save_bin_to_file(void* data, int size, const char* fn);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_H
