// dump.h
#ifndef _DUMP_H
#define _DUMP_H

#include <inttypes.h>
#include "stm32f4xx_hal.h"


#define DUMP_RAM_ADDR    0x20000000
#define DUMP_RAM_SIZE    0x00020000

#define DUMP_CCRAM_ADDR  0x10000000
#define DUMP_CCRAM_SIZE  0x00010000

#define DUMP_REGS_ADDR   0x1000ff00
#define DUMP_REGS_SIZE   0x00000100

// CFSR 0xE000ED28
// HFSR 0xE000ED2C
// DFSR 0xE000ED30
// BFAR 0xE000ED38
// AFSR 0xE000ED3C
#define DUMP_REGS_TO_CCRAM() { \
	memset(DUMP_REGS_ADDR, 0, DUMP_REGS_SIZE);\
	(*((volatile uint32_t*)(DUMP_REGS_ADDR + 0x00))) = SCB->CFSR;\
	(*((volatile uint32_t*)(DUMP_REGS_ADDR + 0x04))) = SCB->HFSR;\
	(*((volatile uint32_t*)(DUMP_REGS_ADDR + 0x08))) = SCB->DFSR;\
	(*((volatile uint32_t*)(DUMP_REGS_ADDR + 0x0c))) = SCB->BFAR;\
	(*((volatile uint32_t*)(DUMP_REGS_ADDR + 0x10))) = SCB->AFSR;\
}


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void dump_to_xflash(void);

extern int dump_save_xflash_to_usb(const char* fn);

extern void dump_hardfault_test(void);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_H
