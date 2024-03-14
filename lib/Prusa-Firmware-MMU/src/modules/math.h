/// @file math.h
#pragma once
#include "../config/config.h"

namespace modules {

/// Specialized math operations
namespace math {

/// (intIn1 * intIn2) >> 8
static inline uint16_t mulU8X16toH16(const uint8_t charIn1, const uint16_t intIn2) {
    uint16_t intRes;
#if !defined(__AVR__) || defined(NO_ASM)
    intRes = ((uint32_t)charIn1 * (uint32_t)intIn2) >> 8;
#else
    asm volatile(
        "mul %B2, %A1    \n\t"
        "movw %0, r0     \n\t"
        "mul %A2, %A1    \n\t"
        "lsl r0          \n\t" //push MSB to carry for rounding
        "adc %A0, r1     \n\t" //add with carry (for rounding)
        "clr r1          \n\t" //make r1 __zero_reg__ again
        "adc %B0, r1     \n\t" //propagate carry of addition (add 0 with carry)
        : "=&r"(intRes)
        : "r"(charIn1), "r"(intIn2)
        : "r0", "r1" //clobbers: Technically these are either scratch registers or always 0 registers, but I'm making sure the compiler knows just in case.
    );
#endif
    return intRes;
}

/// (longIn1 * longIn2) >> 24
static inline uint16_t mulU24X24toH16(const uint32_t &longIn1, const uint32_t &longIn2) {
    uint16_t intRes;
#if !defined(__AVR__) || defined(NO_ASM)
    intRes = ((uint64_t)longIn1 * (uint64_t)longIn2) >> 24;
#else
    asm volatile(
        "clr r26         \n\t"
        "mul %A1, %B2    \n\t"
        "mov r27, r1     \n\t"
        "mul %B1, %C2    \n\t"
        "movw %A0, r0    \n\t"
        "mul %C1, %C2    \n\t"
        "add %B0, r0     \n\t"
        "mul %C1, %B2    \n\t"
        "add %A0, r0     \n\t"
        "adc %B0, r1     \n\t"
        "mul %A1, %C2    \n\t"
        "add r27, r0     \n\t"
        "adc %A0, r1     \n\t"
        "adc %B0, r26    \n\t"
        "mul %B1, %B2    \n\t"
        "add r27, r0     \n\t"
        "adc %A0, r1     \n\t"
        "adc %B0, r26    \n\t"
        "mul %C1, %A2    \n\t"
        "add r27, r0     \n\t"
        "adc %A0, r1     \n\t"
        "adc %B0, r26    \n\t"
        "mul %B1, %A2    \n\t"
        "add r27, r1     \n\t"
        "adc %A0, r26    \n\t"
        "adc %B0, r26    \n\t"
        "lsl r27         \n\t"
        "adc %A0, r26    \n\t"
        "adc %B0, r26    \n\t"
        "clr r1          \n\t"
        : "=&r"(intRes)
        : "d"(longIn1), "d"(longIn2)
        : "r0", "r1", "r26", "r27");
#endif
    return intRes;
}

} // namespace math
} // namespace modules
