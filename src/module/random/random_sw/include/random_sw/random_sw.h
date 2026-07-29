/// @file
/// The implementation is in random_sw.cpp, depending on whether the target wants to use the HW RNG or not
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
    #define RAND_SW_DECL extern "C"
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define RAND_SW_DECL
#endif

/// Generates a 32-bit random number using a SW RNG.
/// This function is ISR-safe.
/// !!! Not cryptographically safe.
RAND_SW_DECL uint32_t rand_u_sw();

/// Seeds the SW RNG.
///
/// Every board otherwise starts from the same compiled-in state, which is a problem for anything
/// that exists to de-phase boards from each other. Seed from something board-specific, such as the
/// MCU unique ID. Any seed value works.
RAND_SW_DECL void rand_seed_sw(uint32_t seed);
