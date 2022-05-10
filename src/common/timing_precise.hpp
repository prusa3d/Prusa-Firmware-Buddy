/**
 * This file was copied from Marlin/Marlin/HAL/shared/Delay.h
 *
 * @file
 */
#pragma once

/**
 * Busy wait delay cycles routines:
 *
 *  DELAY_CYCLES(count): Delay execution in cycles
 *  DELAY_NS(count): Delay execution in nanoseconds
 *  DELAY_US(count): Delay execution in microseconds
 */

#include "../../include/main.h"
#include <stdint.h>

#define FORCE_INLINE __attribute__((always_inline)) inline

#if defined(__arm__) || defined(__thumb__)

    #if __CORTEX_M == 7
        #define PENDING(NOW, SOON) ((int32_t)(NOW - (SOON)) < 0)

// Cortex-M7 can use the cycle counter of the DWT unit
// http://www.anthonyvh.com/2017/05/18/cortex_m-cycle_counter/

FORCE_INLINE static void enableCycleCounter() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Unlock DWT.
    DWT->LAR = 0xC5ACCE55;

    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

FORCE_INLINE volatile uint32_t getCycleCount() { return DWT->CYCCNT; }

FORCE_INLINE static void DELAY_CYCLES(const uint32_t x) {
    const uint32_t endCycles = getCycleCount() + x;
    while (PENDING(getCycleCount(), endCycles)) {
    }
}

        #undef PENDING
    #else

    // https://blueprints.launchpad.net/gcc-arm-embedded/+spec/delay-cycles

        #define nop() __asm__ __volatile__("nop;\n\t" :: \
                                               :)

FORCE_INLINE static void __delay_4cycles(uint32_t cy) { // +1 cycle
        #if ARCH_PIPELINE_RELOAD_CYCLES < 2
            #define EXTRA_NOP_CYCLES A("nop")
        #else
            #define EXTRA_NOP_CYCLES ""
        #endif

    __asm__ __volatile__(
        A(".syntax unified") // is to prevent CM0,CM1 non-unified syntax
        L("1")
            A("subs %[cnt],#1")
                EXTRA_NOP_CYCLES
                    A("bne 1b")
        : [ cnt ] "+r"(cy) // output: +r means input+output
        :                  // input:
        : "cc"             // clobbers:
    );
}

// Delay in cycles
FORCE_INLINE static void DELAY_CYCLES(uint32_t x) {

    if (__builtin_constant_p(x)) {
        #define MAXNOPS 4

        if (x <= (MAXNOPS)) {
            switch (x) {
            case 4:
                nop();
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
        } else { // because of +1 cycle inside delay_4cycles
            const uint32_t rem = (x - 1) % (MAXNOPS);
            switch (rem) {
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
            if ((x = (x - 1) / (MAXNOPS)))
                __delay_4cycles(x); // if need more then 4 nop loop is more optimal
        }
        #undef MAXNOPS
    } else if ((x >>= 2))
        __delay_4cycles(x);
}
        #undef nop

    #endif

#elif defined(__AVR__)

    #define nop() __asm__ __volatile__("nop;\n\t" :: \
                                           :)

FORCE_INLINE static void __delay_4cycles(uint8_t cy) {
    __asm__ __volatile__(
        L("1")
            A("dec %[cnt]")
                A("nop")
                    A("brne 1b")
        : [ cnt ] "+r"(cy) // output: +r means input+output
        :                  // input:
        : "cc"             // clobbers:
    );
}

// Delay in cycles
FORCE_INLINE static void DELAY_CYCLES(uint16_t x) {

    if (__builtin_constant_p(x)) {
    #define MAXNOPS 4

        if (x <= (MAXNOPS)) {
            switch (x) {
            case 4:
                nop();
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
        } else {
            const uint32_t rem = (x) % (MAXNOPS);
            switch (rem) {
            case 3:
                nop();
            case 2:
                nop();
            case 1:
                nop();
            }
            if ((x = (x) / (MAXNOPS)))
                __delay_4cycles(x); // if need more then 4 nop loop is more optimal
        }

    #undef MAXNOPS
    } else if ((x >>= 2))
        __delay_4cycles(x);
}
    #undef nop

#elif defined(ESP32)

FORCE_INLINE static void DELAY_CYCLES(uint32_t x) {
    unsigned long ccount, stop;

    __asm__ __volatile__("rsr     %0, ccount"
                         : "=a"(ccount));

    stop = ccount + x; // This can overflow

    while (ccount < stop) { // This doesn't deal with overflows
        __asm__ __volatile__("rsr     %0, ccount"
                             : "=a"(ccount));
    }
}

#elif defined(__PLAT_LINUX__)

// specified inside platform

#else

    #error "Unsupported MCU architecture"

#endif

// Delay in nanoseconds
#define DELAY_NS(x) DELAY_CYCLES((x) * (ConstexprSystemCoreClock() / 1000000UL) / 1000UL)

// Delay in microseconds
#define DELAY_US(x) DELAY_CYCLES((x) * (ConstexprSystemCoreClock() / 1000000UL))
