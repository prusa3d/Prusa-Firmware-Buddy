#pragma once

#include <stdint.h>
#include <limits.h>

// THe implementation is in random_(sw/hw).cpp, depending on whether the target wants to use the HW RNG or not

#ifdef __cplusplus
    #define RAND_DECL extern "C"
#else
    #define RAND_DECL
#endif

/// Generates a 32-bit random number using a HW RNG. Cryptographically strong.
/// !!! Use this function instead of rand (which is disabled).
/// !!! Use this function instead of HAL_RNG_GenerateRandomNumber.
RAND_DECL uint32_t rand_u();

/// Generates a random number in the range of [0,1)
inline float rand_f() {
    return float(rand_u()) / float(UINT_MAX);
}
