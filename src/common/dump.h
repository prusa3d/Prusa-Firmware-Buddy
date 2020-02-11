// dump.h
#ifndef _DUMP_H
#define _DUMP_H

#include <inttypes.h>
#include "stm32f4xx_hal.h"

#define DUMP_TYPE_NONE         0x00
#define DUMP_TYPE_UNDEFINED    0xff
#define DUMP_TYPE_SNAPSHOT     0x01
#define DUMP_TYPE_HARDFAULT    0x02

// dumped ram area (128kb)
#define DUMP_RAM_ADDR    0x20000000
#define DUMP_RAM_SIZE    0x00020000

// dumped ccram area (64kb), last 256 bytes used for register dump etc.
#define DUMP_CCRAM_ADDR  0x10000000
#define DUMP_CCRAM_SIZE  0x00010000

// dumped otp area (32kb)
#define DUMP_OTP_ADDR  0x1FFF0000
#define DUMP_OTP_SIZE  0x00008000

// dumped flash area (1024kb)
#define DUMP_FLASH_ADDR  0x08000000
#define DUMP_FLASH_SIZE  0x00100000

// general registers stored to ccram
// r0-r12, sp, lr, pc - 64 bytes
// xpsr, fpcsr, PRIMASK, BASEPRI, FAULTMASK, CONTROL, MSP, PSP - 32 bytes
#define DUMP_REGS_GEN_ADDR   0x1000ff00
#define DUMP_REGS_GEN_SIZE   0x00000060
// scb registers stored to ccram (140 bytes)
#define DUMP_REGS_SCB_ADDR   0x1000ff60
#define DUMP_REGS_SCB_SIZE   0x0000008c

#define DUMP_REGS_GEN_FAULT() \
    asm volatile( \
    "   mov r3, lr             \n" /* save lrexc */ \
    "    tst lr, #4             \n" \
    "    ite eq                 \n" \
    "    mrseq r0, msp          \n" /* msp->r0  */ \
    "    mrsne r0, psp             \n" /* psp->r0  */ \
    "    ldr r1, =0x1000ff00    \n" /* hardcoded ccram addres - todo: use macro  */ \
    "    ldr r2, [r0, #0x00]    \n" /* load r0 from stack frame  */ \
    "    str r2, [r1, #0x00]    \n" /* store r0 to ccram  */ \
    "    ldr r2, [r0, #0x04]    \n" /* r1  */ \
    "    str r2, [r1, #0x04]    \n" \
    "    ldr r2, [r0, #0x08]    \n" /* r2  */ \
    "    str r2, [r1, #0x08]    \n" \
    "    ldr r2, [r0, #0x0c]    \n" /* r3  */ \
    "    str r2, [r1, #0x0c]    \n" \
    "    str r4, [r1, #0x10]    \n" /* r4  */ \
    "    str r5, [r1, #0x14]    \n" /* r5  */ \
    "    str r6, [r1, #0x18]    \n" /* r6  */ \
    "    str r7, [r1, #0x1c]    \n" /* r7  */ \
    "    str r8, [r1, #0x20]    \n" /* r8  */ \
    "    str r9, [r1, #0x24]    \n" /* r9  */ \
    "    str r10, [r1, #0x28]   \n" /* r10  */ \
    "    str r11, [r1, #0x2c]   \n" /* r11  */ \
    "    str r12, [r1, #0x30]   \n" /* r12  */ \
    "    str r0, [r1, #0x34]    \n" /* store sp (r0) to ccram  */ \
    "    ldr r2, [r0, #0x14]    \n" /* lr  */ \
    "    str r2, [r1, #0x38]    \n" \
    "    ldr r2, [r0, #0x18]    \n" /* pc  */ \
    "    str r2, [r1, #0x3c]    \n" \
    "    ldr r2, [r0, #0x1c]    \n" /* xpsr  */ \
    "    str r2, [r1, #0x40]    \n" \
    "    mrs r2, PRIMASK        \n" /* PRIMASK  */ \
    "    str r2, [r1, #0x44]    \n" \
    "    mrs r2, BASEPRI        \n" /* BASEPRI  */ \
    "    str r2, [r1, #0x48]    \n" \
    "    mrs r2, FAULTMASK      \n" /* FAULTMASK  */ \
    "    str r2, [r1, #0x4c]    \n" \
    "    mrs r2, CONTROL        \n" /* CONTROL  */ \
    "    str r2, [r1, #0x50]    \n" \
    "    mrs r2, MSP            \n" /* MSP  */ \
    "    str r2, [r1, #0x54]    \n" \
    "    mrs r2, PSP            \n" /* PSP  */ \
    "    str r2, [r1, #0x58]    \n" \
    "    str r3, [r1, #0x5c]    \n" /* lrexc  */\
)


#define DUMP_HARDFAULT_TO_XFLASH() { DUMP_REGS_GEN_FAULT(); dump_to_xflash(); }


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


extern void dump_to_xflash();

extern int dump_save_to_usb(const char* fn);


#ifdef _DEBUG

extern void dump_hardfault_test_0(void);

extern void dump_hardfault_test_1(void);

#endif //_DEBUG


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_H
