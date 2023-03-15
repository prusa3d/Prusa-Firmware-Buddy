#pragma once
///
/// @file board.h
///
/// Macros to differentiate between our Buddy boards
///
/// Usage:
///
///    #if BOARD_IS_BUDDY
///       ... code ...
///    #elif BOARD_IS_XBUDDY
///       ... code ...
///    #else
///       #error Unsupported board
///    #endif
///
///
///    #if BOARD_IS_BUDDY
///       ... code ...
///    #elif BOARD_IS_XBUDDY && BOARD_VER_LOWER_OR_EQUAL_TO(0, 2, 0)
///       ... code for xBuddy 0.2.0 and lower
///    #elif BOARD_IS_XBUDDY
///       ... code for the latest xBuddy
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
/// BOARD_VERSION_MAJOR
/// BOARD_VERSION_MINOR
/// BOARD_VERSION_PATCH
///

#define BOARD_BUDDY      1
#define BOARD_XBUDDY     2
#define BOARD_XLBUDDY    3
#define BOARD_DWARF      4
#define BOARD_MODULARBED 5

#if defined(BOARD) && BOARD == BOARD_BUDDY
    #define BOARD_IS_BUDDY   1
    #define BOARD_IS_XBUDDY  0
    #define BOARD_IS_XLBUDDY 0
    #define BOARD_STRING     "Buddy"
#elif defined(BOARD) && BOARD == BOARD_XBUDDY
    #define BOARD_IS_BUDDY   0
    #define BOARD_IS_XBUDDY  1
    #define BOARD_IS_XLBUDDY 0
    #define BOARD_STRING     "XBuddy"
#elif defined(BOARD) && BOARD == BOARD_XLBUDDY
    #define BOARD_IS_BUDDY   0
    #define BOARD_IS_XBUDDY  0
    #define BOARD_IS_XLBUDDY 1
    #define BOARD_STRING     "XLBuddy"
#elif defined(BOARD) && BOARD == BOARD_DWARF
    #define BOARD_IS_DWARF 1
    #define BOARD_STRING   "Dwarf"
#elif defined(BOARD) && BOARD == BOARD_MODULARBED
    #define BOARD_IS_MODULARBED 1
    #define BOARD_STRING        "ModularBed"
#else
    #error Please define the BOARD macro
#endif

#if !defined(BOARD_VERSION_MAJOR) || !defined(BOARD_VERSION_MINOR) || !defined(BOARD_VERSION_PATCH)
    #error Please define the BOARD_VERSION_MAJOR/MINOR/PATCH macros
#endif

#define BOARD_VER_HIGHER_THAN(major, minor, patch)        ((BOARD_VERSION_MAJOR > (major)) || (BOARD_VERSION_MAJOR == (major) && BOARD_VERSION_MINOR > (minor)) || (BOARD_VERSION_MAJOR == (major) && BOARD_VERSION_MINOR == (minor) && BOARD_VERSION_PATCH > (patch)))
#define BOARD_VER_LOWER_THAN(major, minor, patch)         ((BOARD_VERSION_MAJOR < (major)) || (BOARD_VERSION_MAJOR == (major) && BOARD_VERSION_MINOR < (minor)) || (BOARD_VERSION_MAJOR == (major) && BOARD_VERSION_MINOR == (minor) && BOARD_VERSION_PATCH < (patch)))
#define BOARD_VER_EQUAL_TO(major, minor, patch)           (BOARD_VERSION_MAJOR == (major) && BOARD_VERSION_MINOR == (minor) && BOARD_VERSION_PATCH == (patch))
#define BOARD_VER_HIGHER_OR_EQUAL_TO(major, minor, patch) (BOARD_VER_HIGHER_THAN(major, minor, patch) || BOARD_VER_EQUAL_TO(major, minor, patch))
#define BOARD_VER_LOWER_OR_EQUAL_TO(major, minor, patch)  (BOARD_VER_LOWER_THAN(major, minor, patch) || BOARD_VER_EQUAL_TO(major, minor, patch))
