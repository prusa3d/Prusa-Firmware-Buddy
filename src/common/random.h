#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

// THe implementation is in random_(sw/hw).cpp, depending on whether the target wants to use the HW RNG or not

#ifdef __cplusplus
    #define RAND_DECL extern "C"
#else
    #define RAND_DECL
#endif

/// Generates a 32-bit random number using a HW RNG.
/// !!! Use this function instead of rand (which is disabled).
/// !!! Use this function instead of HAL_RNG_GenerateRandomNumber.
/// !!! This function is not ISR-safe
RAND_DECL uint32_t rand_u();

/// Generates a 32-bit random number using a SW RNG.
/// This function is ISR-safe.
/// !!! Not cryptographically safe.
RAND_DECL uint32_t rand_u_sw();

/// Cryptographically secure version of rand_u. Can fail.
/// !!! This function is not ISR-safe
///
/// Returns true on success and stores the value to out.
///
/// Failure is mostly theoretical concern / possibly HW issue.
RAND_DECL bool rand_u_secure(uint32_t *out);

/// Converts randomly generated uint32_t in the range of [0,1)
inline float rand_f_from_u(uint32_t rand_u_generated_number) {
    return float(rand_u_generated_number) / float(UINT_MAX);
}
