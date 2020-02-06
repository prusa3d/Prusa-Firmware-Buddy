// dump.h
#ifndef _DUMP_H
#define _DUMP_H

#include <inttypes.h>
#include <stdio.h>


#define DUMP_RAM_ADDR	0x20000000
#define DUMP_RAM_SIZE	0x00020000
#define DUMP_CCRAM_ADDR	0x10000000
#define DUMP_CCRAM_SIZE	0x00010000
#define DUMP_OTP_ADDR	0x1fff0000
#define DUMP_OTP_SIZE	0x00008000
#define DUMP_FLASH_ADDR	0x08000000
#define DUMP_FLASH_SIZE	0x00100000


typedef struct _dump_t
{
	uint8_t* ram;
	uint8_t* ccram;
	uint8_t* otp;
	uint8_t* flash;
} dump_t;



#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void dump_free(dump_t* pd);

extern int dump_load(dump_t* pd, const char* fn);

extern int dump_save_bin_to_file(void* data, int size, const char* fn);

extern void dump_print_hardfault_simple(uint32_t* gen_regs, uint32_t* scb_regs);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_H
