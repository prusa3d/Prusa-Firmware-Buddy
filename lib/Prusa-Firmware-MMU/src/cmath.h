/// @file cmath.h
// Provide an uniform interface for basic math functions between AVR libc and newer
// standards that support <cmath>
#pragma once

#ifndef __AVR__
#include <cmath> // abs

#include <algorithm>
using std::max;
using std::min;
#else

// AVR libc doesn't support cmath
#include <math.h>

template <typename T>
static inline const T min(const T a, const T b) {
    return a <= b ? a : b;
}

template <typename T>
static inline const T max(const T a, const T b) {
    return a > b ? a : b;
}

template <typename T>
static inline const T abs(const T n) {
    return n < 0 ? -n : n;
}

static inline const int16_t abs(const int16_t n) {
    // Use builtin function only when possible
    return __builtin_abs((n));
}

#endif
