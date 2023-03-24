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
        "clr r26        \n\t"
        "mul %A1, %B2   \n\t"
        "movw %A0, r0   \n\t"
        "mul %A1, %A2   \n\t"
        "add %A0, r1    \n\t"
        "adc %B0, r26   \n\t"
        "lsr r0         \n\t"
        "adc %A0, r26   \n\t"
        "adc %B0, r26   \n\t"
        "clr r1         \n\t"
        : "=&r"(intRes)
        : "d"(charIn1), "d"(intIn2)
        : "r26");
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
        "lsr r27         \n\t"
        "adc %A0, r26    \n\t"
        "adc %B0, r26    \n\t"
        "clr r1          \n\t"
        : "=&r"(intRes)
        : "d"(longIn1), "d"(longIn2)
        : "r26", "r27");
#endif
    return intRes;
}

} // namespace math
} // namespace modules
