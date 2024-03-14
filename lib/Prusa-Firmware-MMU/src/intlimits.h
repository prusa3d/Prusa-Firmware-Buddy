/// @file intlimits.h
#pragma once
#ifndef __AVR__
#include <limits>
#else

// A minimal std::numeric_limits for platforms that lack one
#include <stddef.h>
#include <stdint.h>

namespace std {

template <typename T>
class numeric_limits;

template <>
class numeric_limits<uint8_t> {
public:
    static constexpr size_t max() { return UINT8_MAX; }
};

} // namespace std

#endif
