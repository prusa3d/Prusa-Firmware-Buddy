// dump.h
#pragma once

#include <inttypes.h>
#include <stdint.h>
#include "client_fsm_types.h"
#include "stm32f4xx_hal.h"
#include "printers.h"
#include "utility_extensions.hpp"

namespace crash_dump {

/// Dump types and flags
enum class DumpType : uint8_t {
    UNDEFINED = 0xff,  // undefined - memory erased/empty/failed to read

    HARDFAULT = 0x01,  // hardfault dump
    IWDGW = 0x02,      // IWDG warning dump
    FATALERROR = 0x04, // fatal error dump
    BSOD = 0x08,       // BSOD dump
    STACK_OVF = 0x10,  // stack overflow dump
    RESERVED = 0x20,   // reserved for future use
    TYPEMASK = 0x3f,   // mask for type

    NOT_SAVED = 0x80,  // dump not saved flag - (unsaved dump cannot be overwritten)
    NOT_DISPL = 0x40,  // dump not displayed after startup
    FLAGMASK = 0xc0,   // mask for flags
};

/// Codes for the message type item of message struct
enum class MsgType : uint8_t {
    RSOD = 0x00,  ///< Red screen of death
    BSOD = 0x01,  ///< Blue screen of death
    EMPTY = 0xff, ///< Nothing dumped
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
#if PRINTER_IS_PRUSA_MINI
    // dumped ram area (128kb)
    RAM_ADDR = 0x20000000,
    RAM_SIZE = 0x00020000,
#elif (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX)
    // dumped ram area (192kb)
    RAM_ADDR = 0x20000000,
    RAM_SIZE = 0x00030000,
#else
    #error "Unknown PRINTER_TYPE!"
#endif

    // dumped ccram area (64kb), last 256 bytes used for register dump etc.
    CCRAM_ADDR = 0x10000000,
    CCRAM_SIZE = 0x00010000,

    // dumped otp area (32kb)
    OTP_ADDR = 0x1FFF0000,
    OTP_SIZE = 0x00008000,

    // dumped flash area (1024kb)
    FLASH_ADDR = 0x08000000,
    FLASH_SIZE = 0x00100000,
};

// DUMP constants for error message
inline constexpr size_t MSG_TITLE_MAX_LEN { 40 };
inline constexpr size_t MSG_MAX_LEN { 107 };
// general registers stored to ccram
// r0-r12, sp, lr, pc - 64 bytes
// xpsr, fpcsr, PRIMASK, BASEPRI, FAULTMASK, CONTROL, MSP, PSP - 32 bytes
inline constexpr uint32_t REGS_GEN_ADDR = 0x1000ff00;
inline constexpr uint32_t REGS_GEN_SIZE = 0x00000060;
// scb registers stored to ccram (140 bytes)
inline constexpr uint32_t REGS_SCB_ADDR = 0x1000ff60;
inline constexpr uint32_t REGS_SCB_SIZE = 0x0000008c;
// dump info stored to ccram (16 bytes)
inline constexpr uint32_t INFO_ADDR = 0x1000fff0;
inline constexpr uint32_t INFO_SIZE = 0x00000010;

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
inline void __attribute__((naked)) dump_regs_gen_exc_to_ccram() {
    asm volatile(
        "    ldr r1, =0x1000ff00    \n"   /* hardcoded ccram addres - todo: use macro  */
        "    b .dump_continue       \n"   /* skip the .ltorg constants                 */
        "    .ltorg                 \n"   /* put the immediate constant 0x1000ff00 here*/
        ".dump_continue:            \n"
        "    ldr r2, [r0, #0x00]    \n"   /* load r0 from stack frame  */
        "    str r2, [r1, #0x00]    \n"   /* store r0 to ccram  */
        "    ldr r2, [r0, #0x04]    \n"   /* r1  */
        "    str r2, [r1, #0x04]    \n"
        "    ldr r2, [r0, #0x08]    \n"   /* r2  */
        "    str r2, [r1, #0x08]    \n"
        "    ldr r2, [r0, #0x0c]    \n"   /* r3  */
        "    str r2, [r1, #0x0c]    \n"
        "    str r4, [r1, #0x10]    \n"   /* r4  */
        "    str r5, [r1, #0x14]    \n"   /* r5  */
        "    str r6, [r1, #0x18]    \n"   /* r6  */
        "    str r7, [r1, #0x1c]    \n"   /* r7  */
        "    str r8, [r1, #0x20]    \n"   /* r8  */
        "    str r9, [r1, #0x24]    \n"   /* r9  */
        "    str r10, [r1, #0x28]   \n"   /* r10  */
        "    str r11, [r1, #0x2c]   \n"   /* r11  */
        "    str r12, [r1, #0x30]   \n"   /* r12  */
        "    str r0, [r1, #0x34]    \n"   /* store sp (r0) to ccram  */
        "    ldr r2, [r0, #0x14]    \n"   /* lr  */
        "    str r2, [r1, #0x38]    \n"
        "    ldr r2, [r0, #0x18]    \n"   /* pc  */
        "    str r2, [r1, #0x3c]    \n"
        "    ldr r2, [r0, #0x1c]    \n"   /* xpsr  */
        "    str r2, [r1, #0x40]    \n"
        "    mrs r2, PRIMASK        \n"   /* PRIMASK  */
        "    str r2, [r1, #0x44]    \n"
        "    mrs r2, BASEPRI        \n"   /* BASEPRI  */
        "    str r2, [r1, #0x48]    \n"
        "    mrs r2, FAULTMASK      \n"   /* FAULTMASK  */
        "    str r2, [r1, #0x4c]    \n"
        "    mrs r2, CONTROL        \n"   /* CONTROL  */
        "    str r2, [r1, #0x50]    \n"
        "    mrs r2, MSP            \n"   /* MSP  */
        "    str r2, [r1, #0x54]    \n"
        "    mrs r2, PSP            \n"   /* PSP  */
        "    str r2, [r1, #0x58]    \n"
        "    str r3, [r1, #0x5c]    \n"); /* lrexc  */
}

// fill dumpinfo
#define DUMP_INFO_TO_CCRAM(type) \
    *((crash_dump::DumpType *)crash_dump::INFO_ADDR) = type | crash_dump::DumpType::NOT_SAVED | crash_dump::DumpType::NOT_DISPL;

// perform hardfault dump (directly from HardFault_Handler that must be "naked")
#define DUMP_HARDFAULT_TO_CCRAM()                            \
    {                                                        \
        DUMP_REGS_GEN_FAULT_BEGIN();                         \
        crash_dump::dump_regs_gen_exc_to_ccram();            \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::HARDFAULT); \
    }

// perform iwdg warning dump (from wdt_iwdg_warning_cb, depth is exception MSP offset from current MSP)
#define DUMP_IWDGW_TO_CCRAM(depth)                       \
    {                                                    \
        DUMP_REGS_GEN_IWDGW_BEGIN(depth);                \
        crash_dump::dump_regs_gen_exc_to_ccram();        \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::IWDGW); \
    }

// perform fatal error dump
#define DUMP_FATALERROR_TO_CCRAM()                            \
    {                                                         \
        DUMP_REGS_GEN_FAULT_BEGIN();                          \
        crash_dump::dump_regs_gen_exc_to_ccram();             \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::FATALERROR); \
    }

// perform bsod dump (directly from _bsod)
#define DUMP_BSOD_TO_CCRAM()                            \
    {                                                   \
        DUMP_REGS_GEN_FAULT_BEGIN();                    \
        crash_dump::dump_regs_gen_exc_to_ccram();       \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::BSOD); \
    }

// perform stack overflow dump (from vApplicationStackOverflowHook())
#define DUMP_STACK_OVF_TO_CCRAM()                            \
    {                                                        \
        DUMP_REGS_GEN_FAULT_BEGIN();                         \
        crash_dump::dump_regs_gen_exc_to_ccram();            \
        DUMP_INFO_TO_CCRAM(crash_dump::DumpType::STACK_OVF); \
    }

/**
 * @brief Check if dump was saved.
 * @return true for saved
 */
bool dump_is_valid();

/**
 * @brief Check if dump was already displayed.
 * @return true for displayed
 */
bool dump_is_displayed();

/**
 * @brief Get dump type.
 * @return UNDEFINED - either there is no dump or failed to read
 */
DumpType dump_get_type();

/**
 * @brief Erase dump.
 */
void dump_reset();

/**
 * @brief Set dump as displayed.
 */
void dump_set_displayed();

/**
 * @brief Get RAM data from dump.
 * @param pRAM will be filled with RAM data
 * @param addr address of RAM data in dump
 * @param size size of pRAM [B]
 * @return size of filled data [B]
 */
size_t load_dump_RAM(void *pRAM, uint32_t addr, size_t size);

/**
 * @brief Get SCB register data from dump.
 * @param pRegsSCB will be filled with register data
 * @param size size of pRegsSCB [B]
 * @return size of filled data [B]
 */
size_t load_dump_regs_SCB(void *pRegsSCB, size_t size);

/**
 * @brief Get generic register data from dump.
 * @param pRegsGEN will be filled with register data
 * @param size size of pRegsGEN [B]
 * @return size of filled data [B]
 */
size_t load_dump_regs_GEN(void *pRegsGEN, size_t size);

/**
 * @brief Store dump to USB.
 * @param fn Filename to store dump to
 * @return true on success
 */
bool save_dump_to_usb(const char *fn);

/**
 * @brief Save dump to XFLASH.
 * Use one of DUMP_HARDFAULT_TO_CCRAM() before this to get registers to CCRAM.
 */
void save_dump();

/**
 * @brief Dump error message to XFLASH.
 * This is used for redscreen error message and for BSOD error message.
 * @param invalid RSOD or BSOD, erased value means message invalid
 * @param error_code enum from the error list, not used for BSOD (use 0 or ERR_UNDEF instead)
 * @note This uses uint16_t to minimize dependencies. Include <error_codes.hpp> and use ftrstd::to_underlying(ERR_WHATEVER).
 * @param error longer error message
 * @param title shorter error title, or file and line for BSOD
 */
void save_message(MsgType invalid, uint16_t error_code, const char *error, const char *title);

/**
 * @brief Copy error message from XFLASH.
 * @param msg_dst [out] - will be filled with address to dumped error message
 * @param msg_dst_size [in] - size of passed message buffer
 * @param title_dst [out] - will be filled with address to dumped error title
 * @param msg_dst_size [in] - size of passed title buffer
 * @retval true - valid read
 * @retval false - read error occurred
 */
bool load_message(char *msg_dst, size_t msg_dst_size, char *tit_dst, size_t tit_dst_size);

/**
 * @brief Check if error message in XFLASH is valid.
 * @return type of the error message structure or EMPTY
 */
MsgType message_get_type();

/**
 * @brief Returns if error message was already displayed.
 * @return true if it was
 */
bool message_is_displayed();

/**
 * @brief Set message as displayed.
 */
void message_set_displayed();

/**
 * @brief Get RSOD error code.
 * @note This returns uint16_t to minimize dependencies. Include <error_codes.hpp> and use ErrCode(load_message_error_code()).
 * @return error code,
 */
uint16_t load_message_error_code();

} // namespace crash_dump
