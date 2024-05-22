/// @file debug.cpp
#include "debug.h"

#if defined(DEBUG_LOGIC) || defined(DEBUG_MODULES) || defined(DEBUG_HAL)

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace debug {

#ifdef DEBUG_LOGIC
const char logic[] PROGMEM = "log:";
#endif

#ifdef DEBUG_MODULES
const char modules[] PROGMEM = "mod:";
#endif

#ifdef DEBUG_HAL
const char hal[] PROGMEM = "hal:";
#endif

void dbg_usb(const char *layer_P, const char *s) {
    fputs_P(layer_P, stdout);
    puts(s);
}

void dbg_usb_P(const char *layer_P, const char *s_P) {
    fputs_P(layer_P, stdout);
    puts_P(s_P);
}

void dbg_usb_fP(const char *layer_P, const char *fmt_P, ...) {
    va_list argptr;
    va_start(argptr, fmt_P);
    fputs_P(layer_P, stdout);
    vfprintf_P(stdout, fmt_P, argptr);
    putchar('\n');
    va_end(argptr);
}

} // namespace debug

#endif
