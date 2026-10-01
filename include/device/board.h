#pragma once
///
/// @file board.h
///
/// Macros to differentiate between our Buddy boards
///
/// Usage:
///
///    #if BOARD_IS_BUDDY()
///       ... code ...
///    #elif BOARD_IS_XBUDDY()
///       ... code ...
///    #else
///       #error Unsupported board
///    #endif
///
///
///    #if BOARD_IS_BUDDY()
///       ... code ...
///    #elif BOARD_IS_XBUDDY()
///       ... code ...
///    #else
///       #error Unsupported board
///    #endif
///
///
/// Macros to be defined when invoking the compiler
/// DO NOT USE those macros in your code (if you don't have a really good reason to).
/// USE the macros defined below instead.
///
/// BOARD (e.g. BUDDY_BOARD)
///

#define BOARD_BUDDY()              1
#define BOARD_XBUDDY()             2
#define BOARD_XLBUDDY()            3
#define BOARD_DWARF()              4
#define BOARD_MODULARBED()         5
#define BOARD_XL_DEV_KIT_XLB()     6
#define BOARD_XBUDDY_EXTENSION()   7
#define BOARD_ANFC()               8
#define BOARD_TOOL_OFFSET_SENSOR() 9
#define BOARD_HTCONTROLLER()       10

#if !defined(BOARD)
    #error Please define the BOARD macro
#elif BOARD() == BOARD_BUDDY()
    #define BOARD_IS_BUDDY() 1
#elif BOARD() == BOARD_XBUDDY()
    #define BOARD_IS_XBUDDY() 1
#elif BOARD() == BOARD_XLBUDDY()
    #define BOARD_IS_XLBUDDY() 1
#elif BOARD() == BOARD_DWARF()
    #define BOARD_IS_DWARF() 1
#elif BOARD() == BOARD_MODULARBED()
    #define BOARD_IS_MODULARBED() 1
#elif BOARD() == BOARD_XL_DEV_KIT_XLB()
    #define BOARD_IS_XLBUDDY()        1 // todo: remove, for now xl dev two  boards enabled
    #define BOARD_IS_XL_DEV_KIT_XLB() 1
#elif BOARD() == BOARD_XBUDDY_EXTENSION()
    #define BOARD_IS_XBUDDY_EXTENSION() 1
#elif BOARD() == BOARD_ANFC()
    #define BOARD_IS_ANFC() 1
#elif BOARD() == BOARD_TOOL_OFFSET_SENSOR()
    #define BOARD_IS_TOOL_OFFSET_SENSOR() 1
#elif BOARD() == BOARD_HTCONTROLLER()
    #define BOARD_IS_HTCONTROLLER() 1
#else
    #error BOARD is something weird
#endif

#ifndef BOARD_IS_BUDDY
    #define BOARD_IS_BUDDY() 0
#endif

#ifndef BOARD_IS_XBUDDY
    #define BOARD_IS_XBUDDY() 0
#endif

#ifndef BOARD_IS_XLBUDDY
    #define BOARD_IS_XLBUDDY() 0
#endif

#ifndef BOARD_IS_XL_DEV_KIT_XLB
    #define BOARD_IS_XL_DEV_KIT_XLB() 0
#endif

#ifndef BOARD_IS_DWARF
    #define BOARD_IS_DWARF() 0
#endif

#ifndef BOARD_IS_MODULARBED
    #define BOARD_IS_MODULARBED() 0
#endif

#ifndef BOARD_IS_XBUDDY_EXTENSION
    #define BOARD_IS_XBUDDY_EXTENSION() 0
#endif

#ifndef BOARD_IS_ANFC
    #define BOARD_IS_ANFC() 0
#endif

#ifndef BOARD_IS_TOOL_OFFSET_SENSOR
    #define BOARD_IS_TOOL_OFFSET_SENSOR() 0
#endif

#ifndef BOARD_IS_HTCONTROLLER
    #define BOARD_IS_HTCONTROLLER() 0
#endif
