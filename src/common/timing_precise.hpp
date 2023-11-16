/**
 * This file was copied from Marlin/Marlin/HAL/shared/Delay.h
 * and adapted by removing Marlin's macros and unsupported architectures.
 * Naming convention was changed to be more in line with Buddy
 * coding standard.
 * Macro DELAY_MS was changed to force inline delay_us_precise function.
 * Support for non-constexpr parameter of DELAY_NS_PRECISE was
 * removed as I considered it broken.
 *
 * @file
 */
#pragma once

#include <device/cmsis.h>
#include <stdint.h>
#include <limits>

// Ensure functions are kept optimized also in debug builds to keep timing consistent
#pragma GCC push_options
#pragma GCC optimize("O3")

#define FORCE_INLINE __attribute__((always_inline)) inline

#if defined(__arm__) || defined(__thumb__)

// https://blueprints.launchpad.net/gcc-arm-embedded/+spec/delay-cycles

/**
 * @brief Delay number of multiplies of 4 CPU cycles
 *
 * Implementation detail, use timing_delay_cycles() instead.
 *
 * @param cy number of multiplies of 4 CPU cycles
 */
FORCE_INLINE static void timing_delay_4cycles(uint32_t cy) { // +1 cycle
    #if ARCH_PIPELINE_RELOAD_CYCLES < 2
        #define EXTRA_NOP_CYCLES " nop\n\t"
    #else
        #define EXTRA_NOP_CYCLES ""
    #endif

    __asm__ __volatile__(
        " .syntax unified\n\t" // is to prevent CM0,CM1 non-unified syntax
        "1:\n\t"
        " subs %[cnt],#1\n\t" //
        EXTRA_NOP_CYCLES
        " bne 1b\n\t"
        : [cnt] "+r"(cy) // output: +r means input+output
        : // input:
        : "cc" // clobbers:
    );
    #undef EXTRA_NOP_CYCLES
}

/**
 * @brief Delay number of CPU cycles
 *
 * It is precise to single CPU cycle when x is compile time constant
 * to 4 CPU cycles otherwise.
 *
 * @param x number of CPU cycles
 */
FORCE_INLINE static void timing_delay_cycles(uint32_t x) {
    #define nop() __asm__ __volatile__("nop;\n\t" :: \
                                           :)

    if (__builtin_constant_p(x)) {
    #define MAXNOPS 4

        if (x <= (MAXNOPS)) {
            switch (x) {
            case 4:
                nop();
                [[fallthrough]];
            case 3:
                nop();
                [[fallthrough]];
            case 2:
                nop();
                [[fallthrough]];
            case 1:
                nop();
            }
        } else { // because of +1 cycle inside delay_4cycles
            const uint32_t rem = (x - 1) % (MAXNOPS);
            switch (rem) {
            case 3:
                nop();
                [[fallthrough]];
            case 2:
                nop();
                [[fallthrough]];
            case 1:
                nop();
            }
            if ((x = (x - 1) / (MAXNOPS))) {
                timing_delay_4cycles(x); // if need more then 4 nop loop is more optimal
            }
        }
    #undef MAXNOPS
    } else if ((x >>= 2)) {
        timing_delay_4cycles(x);
    }
    #undef nop
}

    #if __CORTEX_M == 7
        #error "Support removed, you can get it from original Marlin source."
    #endif
#elif defined(__AVR__) || defined(__PLAT_LINUX__)
    #error "Support removed, you can get it from original Marlin source."
#else
    #error "Unsupported MCU architecture"
#endif

/**
 * @param ns time in nanoseconds
 * @return number of CPU cycles
 */
FORCE_INLINE constexpr uint64_t timing_nanoseconds_to_cycles(uint64_t ns) {
    return ((ns * (SYSTEM_CORE_CLOCK / 1000000UL)) / 1000UL);
}

/**
 * @param us time in microseconds
 * @return number of CPU cycles
 */
FORCE_INLINE constexpr uint32_t timing_microseconds_to_cycles(uint32_t us) {
    return (us * (SYSTEM_CORE_CLOCK / 1000000UL));
}

/**
 * @brief Delay nanoseconds
 *
 * Timing precision is single CPU cycle. E.g. 6 ns at 168Mhz
 * It is always guaranteed to return if CPU is running.
 * Correct timing is guaranteed after SystemClock_Config() call if
 * caller can not be interrupted.
 *
 * @param ns time in nanoseconds (compile time constant)
 */

template <uint64_t ns>
FORCE_INLINE static constexpr void delay_ns_precise() {
    static_assert(ns < (std::numeric_limits<uint64_t>::max() / (SYSTEM_CORE_CLOCK / 1000000UL)),
        "ns out of range");
    static_assert(timing_nanoseconds_to_cycles(ns) <= std::numeric_limits<uint32_t>::max(),
        "ns out of range");
    constexpr uint32_t cycles = timing_nanoseconds_to_cycles(ns);
    timing_delay_cycles(cycles);
}

/**
 * @brief Delay microseconds
 *
 * Timing precision is single CPU cycle. E.g. 6 ns at 168Mhz
 * It is always guaranteed to return if CPU is running.
 * Correct timing is guaranteed after SystemClock_Config() call if
 * caller can not be interrupted.
 *
 * @param us time in microseconds (compile time constant)
 */
template <uint64_t us>
FORCE_INLINE static constexpr void delay_us_precise() {
    delay_ns_precise<us * 1000ULL>();
}

/**
 * @brief Delay microseconds
 *
 * Timing precision is 1 microsecond if us is compile time constant and
 * CPU clock is at least 1Mhz. Timing precision is 1 microsecond plus
 * constant delay incurred by uint32_t multiplication otherwise if CPU
 * clock is at least 4Mhz.
 *
 * It is always guaranteed to return if CPU is running.
 * Correct timing is guaranteed after SystemClock_Config() call if
 * caller can not be interrupted.
 *
 * Use delay_us_precise<>()/delay_ns_precise<>() if dealing with compile
 * time constant delay to get range check for free.
 *
 * @param us time in microseconds
 * Maximum range depends on CPU clock. For 168 Mhz it is 25 565 us.
 */
FORCE_INLINE void delay_us_precise(uint32_t us) {
    timing_delay_cycles(timing_microseconds_to_cycles(us));
}

#pragma GCC pop_options
