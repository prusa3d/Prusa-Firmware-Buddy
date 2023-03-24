/// @file debug.h
#pragma once
#include <stdint.h>
#include "config/config.h"

/// Enable DEBUG_LOGIC to compile debugging and error messages (beware of code base size ;) ) for the logic layer
//#define DEBUG_LOGIC

/// Enable DEBUG_LOGIC to compile debugging and error messages (beware of code base size ;) ) for the logic layer
//#define DEBUG_MODULES

/// Enable DEBUG_HAL to compile debugging and error messages (beware of code base size ;) ) for the logic layer
//#define DEBUG_HAL

// workaround non-standard PSTR definition on non-AVR platforms
#ifndef __AVR__
#define PSTR(x) x
#endif

/// Debugging macros and tools
namespace debug {

#if defined(DEBUG_LOGIC) && defined(__AVR__)
extern const char logic[];
#define dbg_logic(x) debug::dbg_usb(debug::logic, x)
#define dbg_logic_P(x) debug::dbg_usb_P(debug::logic, x)
#define dbg_logic_fP(fmt, ...) debug::dbg_usb_fP(debug::logic, fmt, __VA_ARGS__)
#else
#define dbg_logic(x) /* */
#define dbg_logic_P(x) /* */
#define dbg_logic_fP(fmt, ...) /* */
#endif

#if defined(DEBUG_MODULES) && defined(__AVR__)
extern const char modules[];
#define dbg_modules(x) debug::dbg_usb(debug::modules, x)
#define dbg_modules_P(x) debug::dbg_usb_P(debug::modules, x)
#define dbg_modules_fP(fmt, ...) debug::dbg_usb_fP(debug::modules, fmt, __VA_ARGS__)
#else
#define dbg_modules(x) /* */
#define dbg_modules_P(x) /* */
#define dbg_modules_fP(fmt, ...) /* */
#endif

#if defined(DEBUG_HAL) && defined(__AVR__)
extern const char hal[];
#define dbg_hal(x) debug::dbg_usb(debug::hal, x)
#define dbg_hal_P(x) debug::dbg_usb_P(debug::hal, x)
#define dbg_hal_fP(fmt, ...) debug::dbg_usb_fP(debug::hal, fmt, __VA_ARGS__)
#else
#define dbg_hal(x) /* */
#define dbg_hal_P(x) /* */
#define dbg_hal_fP(fmt, ...) /* */
#endif

#if defined(DEBUG_LOGIC) || defined(DEBUG_MODULES) || defined(DEBUG_HAL)
/// Dump an error message onto the usb
/// @param layer PROGMEM string
/// @param s RAM string to be printed
void dbg_usb(const char *layer_P, const char *s);

/// Dump an error message onto the usb
/// @param layer PROGMEM string
/// @param s PROGMEM string to be printed
void dbg_usb_P(const char *layer_P, const char *s_P);

/// Dump an error message onto the usb
/// @param layer PROGMEM string
/// @param fmt PROGMEM format string
/// @param ... arguments passed to the formatting string
void dbg_usb_fP(const char *layer_P, const char *fmt_P, ...);
#endif

} // namespace debug
