// dump.h
#pragma once

#include <inttypes.h>
#include <stdint.h>
#include "client_fsm_types.h"
#include "stm32f4xx_hal.h"
#include "printers.h"
#include "utility_extensions.hpp"

namespace crash_dump {

// dump types and flags
enum class DumpType : uint8_t {
    DUMP_UNDEFINED = 0xff,  // undefined - memory erased/empty/failed to read
    DUMP_HARDFAULT = 0x01,  // hardfault dump
    DUMP_IWDGW = 0x02,      // IWDG warning dump
    DUMP_FATALERROR = 0x04, // fatal error dump
    DUMP_NOT_SAVED = 0x80,  // dump not saved flag - (unsaved dump cannot be overwritten)
    DUMP_NOT_DISPL = 0x40,  // dump not displayed after startup
};

inline DumpType operator|(const DumpType a, const DumpType b) {
    return DumpType(ftrstd::to_underlying(a) | ftrstd::to_underlying(b));
}

inline DumpType operator&(const DumpType a, const DumpType b) {
    return DumpType(ftrstd::to_underlying(a) & ftrstd::to_underlying(b));
}

inline DumpType operator~(const DumpType a) {
    return DumpType(~ftrstd::to_underlying(a));
}

inline bool any(const DumpType a) {
    return ftrstd::to_underlying(a);
}

enum {
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    // dumped ram area (128kb)
    DUMP_RAM_ADDR = 0x20000000,
    DUMP_RAM_SIZE = 0x00020000,
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_XL || PRINTER_TYPE == PRINTER_PRUSA_IXL)
    // dumped ram area (192kb)
    DUMP_RAM_ADDR = 0x20000000,
    DUMP_RAM_SIZE = 0x00030000,
#else
    #error "Unknown PRINTER_TYPE!"
#endif

    // dumped ccram area (64kb), last 256 bytes used for register dump etc.
    DUMP_CCRAM_ADDR = 0x10000000,
    DUMP_CCRAM_SIZE = 0x00010000,

    // dumped otp area (32kb)
    DUMP_OTP_ADDR = 0x1FFF0000,
    DUMP_OTP_SIZE = 0x00008000,

    // dumped flash area (1024kb)
    DUMP_FLASH_ADDR = 0x08000000,
    DUMP_FLASH_SIZE = 0x00100000,
};

// DUMP constants for error message
inline constexpr auto DUMP_MSG_TITLE_MAX_LEN { 20 };
inline constexpr auto DUMP_MSG_MAX_LEN { 107 };
// general registers stored to ccram
// r0-r12, sp, lr, pc - 64 bytes
// xpsr, fpcsr, PRIMASK, BASEPRI, FAULTMASK, CONTROL, MSP, PSP - 32 bytes
inline constexpr uint32_t DUMP_REGS_GEN_ADDR = 0x1000ff00;
inline constexpr uint32_t DUMP_REGS_GEN_SIZE = 0x00000060;
// scb registers stored to ccram (140 bytes)
inline constexpr uint32_t DUMP_REGS_SCB_ADDR = 0x1000ff60;
inline constexpr uint32_t DUMP_REGS_SCB_SIZE = 0x0000008c;
// dump info stored to ccram (16 bytes)
inline constexpr uint32_t DUMP_INFO_ADDR = 0x1000fff0;
inline constexpr uint32_t DUMP_INFO_SIZE = 0x00000010;

// prepare R0 and R3 for DUMP_REGS_GEN_EXC_TO_CCRAM in fault handlers
#define DUMP_REGS_GEN_FAULT_BEGIN()                       \
    asm volatile(                                         \
        "    mov r3, lr             \n" /* save lrexc  */ \
        "    tst lr, #4             \n"                   \
        "    ite eq                 \n"                   \
        "    mrseq r0, MSP          \n" /* MSP -> r0  */  \
        "    mrsne r0, PSP          \n" /* PSP -> r0  */  \
    )

// prepare R0 and R3 for DUMP_REGS_GEN_EXC_TO_CCRAM in IWDG warning callback
#define DUMP_REGS_GEN_IWDGW_BEGIN(depth) \
    asm volatile(                        \
        "    mrs r1, MSP            \n"  \
        "    add r1, #" #depth "    \n"  \
        "    ldr r2, [r1, #0x04]    \n"  \
        "    mov r3, r2             \n"  \
        "    tst r2, #4             \n"  \
        "    ite eq                 \n"  \
        "    moveq r0, r1           \n"  \
        "    mrsne r0, PSP          \n")

// Store general registers from exception to ccram for dump.
// R0 must point to stack frame containing saved R0-R3, R12, LR, PC, xPSR (4x8bytes=32bytes)
// R3 contain lrexc
#define DUMP_REGS_GEN_EXC_TO_CCRAM()                                                    \
    asm volatile(                                                                       \
        "    ldr r1, =0x1000ff00    \n" /* hardcoded ccram addres - todo: use macro  */ \
        "    b .dump_continue       \n" /* skip the .ltorg constants                 */ \
        "    .ltorg                 \n" /* put the immediate constant 0x1000ff00 here*/ \
        ".dump_continue:            \n"                                                 \
        "    ldr r2, [r0, #0x00]    \n" /* load r0 from stack frame  */                 \
        "    str r2, [r1, #0x00]    \n" /* store r0 to ccram  */                        \
        "    ldr r2, [r0, #0x04]    \n" /* r1  */                                       \
        "    str r2, [r1, #0x04]    \n"                                                 \
        "    ldr r2, [r0, #0x08]    \n" /* r2  */                                       \
        "    str r2, [r1, #0x08]    \n"                                                 \
        "    ldr r2, [r0, #0x0c]    \n" /* r3  */                                       \
        "    str r2, [r1, #0x0c]    \n"                                                 \
        "    str r4, [r1, #0x10]    \n" /* r4  */                                       \
        "    str r5, [r1, #0x14]    \n" /* r5  */                                       \
        "    str r6, [r1, #0x18]    \n" /* r6  */                                       \
        "    str r7, [r1, #0x1c]    \n" /* r7  */                                       \
        "    str r8, [r1, #0x20]    \n" /* r8  */                                       \
        "    str r9, [r1, #0x24]    \n" /* r9  */                                       \
        "    str r10, [r1, #0x28]   \n" /* r10  */                                      \
        "    str r11, [r1, #0x2c]   \n" /* r11  */                                      \
        "    str r12, [r1, #0x30]   \n" /* r12  */                                      \
        "    str r0, [r1, #0x34]    \n" /* store sp (r0) to ccram  */                   \
        "    ldr r2, [r0, #0x14]    \n" /* lr  */                                       \
        "    str r2, [r1, #0x38]    \n"                                                 \
        "    ldr r2, [r0, #0x18]    \n" /* pc  */                                       \
        "    str r2, [r1, #0x3c]    \n"                                                 \
        "    ldr r2, [r0, #0x1c]    \n" /* xpsr  */                                     \
        "    str r2, [r1, #0x40]    \n"                                                 \
        "    mrs r2, PRIMASK        \n" /* PRIMASK  */                                  \
        "    str r2, [r1, #0x44]    \n"                                                 \
        "    mrs r2, BASEPRI        \n" /* BASEPRI  */                                  \
        "    str r2, [r1, #0x48]    \n"                                                 \
        "    mrs r2, FAULTMASK      \n" /* FAULTMASK  */                                \
        "    str r2, [r1, #0x4c]    \n"                                                 \
        "    mrs r2, CONTROL        \n" /* CONTROL  */                                  \
        "    str r2, [r1, #0x50]    \n"                                                 \
        "    mrs r2, MSP            \n" /* MSP  */                                      \
        "    str r2, [r1, #0x54]    \n"                                                 \
        "    mrs r2, PSP            \n" /* PSP  */                                      \
        "    str r2, [r1, #0x58]    \n"                                                 \
        "    str r3, [r1, #0x5c]    \n") /* lrexc  */

// fill dumpinfo
#define DUMP_INFO_TO_CCRAM(type) \
    *((DumpType *)DUMP_INFO_ADDR) = type | DumpType::DUMP_NOT_SAVED | DumpType::DUMP_NOT_DISPL;

// perform hardfault dump (directly from HardFault_Handler that must be "naked")
#define DUMP_HARDFAULT_TO_CCRAM()                                 \
    {                                                             \
        DUMP_REGS_GEN_FAULT_BEGIN();                              \
        DUMP_REGS_GEN_EXC_TO_CCRAM();                             \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::DUMP_HARDFAULT); \
    }

// perform iwdg warning dump (from wdt_iwdg_warning_cb, depth is exception MSP offset from current MSP)
#define DUMP_IWDGW_TO_CCRAM(depth)                \
    {                                             \
        DUMP_REGS_GEN_IWDGW_BEGIN(depth);         \
        DUMP_REGS_GEN_EXC_TO_CCRAM();             \
        DUMP_INFO_TO_CCRAM(DumpType::DUMP_IWDGW); \
    }

// perform fatal error dump
#define DUMP_FATALERROR_TO_CCRAM()                     \
    {                                                  \
        DUMP_REGS_GEN_FAULT_BEGIN();                   \
        DUMP_REGS_GEN_EXC_TO_CCRAM();                  \
        DUMP_INFO_TO_CCRAM(DumpType::DUMP_FATALERROR); \
    }

#pragma pack(push)
#pragma pack(1)
typedef struct _dumpinfo_t {
    DumpType type_flags;        //
    unsigned char reserved[15]; // TODO: RTC time, code
} dumpinfo_t;

typedef struct _dumpmessage_t {
    uint8_t not_displayed; // not displayed == 0xFF, displayed == 0x00
    uint8_t invalid;       // valid == 0x00, empty == 0xFF
    uint16_t error_code;   // error_code (0 == unknown error code -> we read dumped message
    char title[DUMP_MSG_TITLE_MAX_LEN];
    char msg[DUMP_MSG_MAX_LEN];
} dumpmessage_t;
#pragma pack(pop)

extern bool dump_in_xflash_is_valid(void);

extern bool dump_in_xflash_is_displayed(void);

extern DumpType dump_in_xflash_get_type(void);

extern void dump_in_xflash_reset(void);

extern void dump_in_xflash_set_displayed(void);

extern unsigned int dump_in_xflash_read_RAM(void *pRAM, unsigned int addr, unsigned int size);

extern unsigned int dump_in_xflash_read_regs_SCB(void *pRegsSCB, unsigned int size);

extern unsigned int dump_in_xflash_read_regs_GEN(void *pRegsGEN, unsigned int size); // TODO: obsolete ?

extern int dump_save_to_usb(const char *fn);

extern void dump_hardfault_test_0(void);

extern int dump_hardfault_test_1(void);

extern void dump_to_xflash(void);

/** Save error message to xflash
 * @param error_code [in] - code for known errors
 * @param error [in] - pointer to dumped error message
 * @param title [in] - pointer to dumped error title
*/
extern void dump_err_to_xflash(uint16_t error_code, const char *error, const char *title);

/** Get pointers to dumped error title and error message
 * @param msg_dst [out] - will be filled with address to dumped error message
 * @param msg_dst_size [in] - size of passed message buffer
 * @param title_dst [out] - will be filled with address to dumped error title
 * @param msg_dst_size [in] - size of passed title buffer
 * @retval true - valid read
 * @retval false - read error occurred
*/
extern bool dump_err_in_xflash_get_message(char *msg_dst, uint16_t msg_dst_size, char *tit_dst, uint16_t tit_dst_size);

/** Returns dumped message valid flag byte
 * @retval true - message is valid
 * @retval false - message space in xflash is empty / not valid
*/
extern bool dump_err_in_xflash_is_valid(void);

/** Returns if error message was already displayed
 * @retval true - Yes
 * @retval false - No
*/
extern bool dump_err_in_xflash_is_displayed(void);

/** Set displayed flag to 'Already displayed' == 0x00 */
extern void dump_err_in_xflash_set_displayed(void);

/** Returns error code
 * @retval 0 - Unknown error code
*/
extern uint16_t dump_err_in_xflash_get_error_code(void);

}
