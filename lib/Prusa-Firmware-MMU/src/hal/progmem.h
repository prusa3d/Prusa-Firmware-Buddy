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

} // namespace progmem
} // namespace hal
