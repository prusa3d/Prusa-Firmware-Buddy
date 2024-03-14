/// @file progmem.h
#pragma once
#include <stdint.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM // ignored
#endif

namespace hal {

/// AVR PROGMEM handling
namespace progmem {

/// read a 16bit word from PROGMEM
static inline uint16_t read_word(const uint16_t *addr) {
#ifndef __AVR__
    return *addr;
#else
    return (uint16_t)pgm_read_word(addr);
#endif
}

/// read a 8bit byte from PROGMEM
static inline uint8_t read_byte(const uint8_t *addr) {
#ifndef __AVR__
    return *addr;
#else
    return (uint8_t)pgm_read_byte(addr);
#endif
}

/// read a ptr from PROGMEM
/// Introduced mainly for compatibility reasons with the unit tests
/// and to hide the ugly reinterpret_casts.
/// Returns a correctly typed pointer: a 16-bit on AVR, but a 64bit address on x86_64
template <typename RT>
static inline RT read_ptr(const void *addr) {
#ifndef __AVR__
    return reinterpret_cast<RT>(*reinterpret_cast<const uint64_t *>(addr));
#else
    return reinterpret_cast<RT>(pgm_read_ptr(addr));
#endif
}

} // namespace progmem
} // namespace hal
