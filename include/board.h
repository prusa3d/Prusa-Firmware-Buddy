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
///    #else
///       #error Unsupported board
///    #endif
///
///
///    #if BOARD_IS_BUDDY && BOARD_VER_LOWER_OR_EQUAL_TO(1, 0, 0)
///       ... code for Buddy 1.0.0 and lower
///    #elif BOARD_IS_BUDDY
///       ... code for the latest Buddy
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

#define BOARD_BUDDY 1

#if defined(BOARD) && BOARD == BOARD_BUDDY
    #define BOARD_IS_BUDDY 1
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
