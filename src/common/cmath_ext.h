/// cmath_ext.h
///
/// Library of flexible functions not defined in standard C99
///
#pragma once

#include <stdlib.h>

// Undefine type-unsafe C definitions (if any)
#undef MAX
#undef MIN

/// \returns maximum of the two
#define MAX(a, b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a >= _b ? _a : _b; })

/// \returns minimum of the two
#define MIN(a, b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a <= _b ? _a : _b; })

/// -1 = negative, 1 = positive, 0 = 0
#define SIGN0(x) \
    ({ __typeof__ (x) _x = (x); \
    (_x > 0) - (_x < 0); })

/// -1 = negative, 1 = positive or 0
#define SIGN1(x) \
    ({ __typeof__ (x) _x = (x); \
    (_x >= 0) - (_x < 0); })

/// swap values
#define SWAP(a, b) \
    ({ __typeof__ (a) c = (a); \
        a = (b); \
        b = c; })

/// Compares two float numbers whether they are almost equal or not
/// \param max_abs_diff is maximal absolute difference allowed for equality
#define nearlyEqual(a, b, max_abs_diff) \
    ({ __typeof__ (a) d = (a)-(b); \
    d <= max_abs_diff && d >= -max_abs_diff; })

/// \returns square of the value
#define SQR(a) \
    ({ __typeof__ (a) a_ = (a); \
        (a_ * a_); })

/// \returns number of items in the array
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/// \returns true if \param value is between
/// \param min and \param max including limits
#define IS_IN_RANGE(value, min, max) \
    ({ __typeof__ (value) a_ = (value); \
        (min <= a_ && a_ <= max); })

/// \returns false if \param value is between
/// \param min and \param max including limits
#define IS_OUT_OF_RANGE(value, min, max) \
    ({ __typeof__ (value) a_ = (value); \
        ( a_ < min || max < a_); })

/// Saturates value
/// \returns min if value is less than min
/// \returns max if value is more than min
/// \returns value otherwise
#define CLAMP(value, min, max) \
    ({ __typeof__ (value) a_ = (value); \
        ( a_ < min ? min : (a_ <= max ? a_ : max)); })
