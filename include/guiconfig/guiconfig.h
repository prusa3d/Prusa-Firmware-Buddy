#pragma once

#include <device/board.h>

///////////////////////////
// display type selector //
///////////////////////////

/// Standard 480x320 display, used on Mk4, XL, ...
#define DISPLAY_TYPE_LARGE 0

/// Smaller 240x320 display, used on Mini
#define DISPLAY_TYPE_MINI 1

/// Not sure what this one does
#define DISPLAY_TYPE_MOCK 2

#if BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
    #define DISPLAY_TYPE() DISPLAY_TYPE_LARGE
#elif BOARD_IS_BUDDY()
    #define DISPLAY_TYPE() DISPLAY_TYPE_MINI
#else
    #define DISPLAY_TYPE() DISPLAY_TYPE_MOCK
#endif

#if DISPLAY_TYPE() == DISPLAY_TYPE_LARGE
    #define HAS_LARGE_DISPLAY()   1
    #define HAS_ILI9488_DISPLAY() 1
#else
    #define HAS_LARGE_DISPLAY()   0
    #define HAS_ILI9488_DISPLAY() 0
#endif

#if DISPLAY_TYPE() == DISPLAY_TYPE_MINI
    #define HAS_MINI_DISPLAY()   1
    #define HAS_ST7789_DISPLAY() 1
#else
    #define HAS_MINI_DISPLAY()   0
    #define HAS_ST7789_DISPLAY() 0
#endif

#if DISPLAY_TYPE() == DISPLAY_TYPE_MOCK
    #define HAS_MOCK_DISPLAY() 1
#else
    #define HAS_MOCK_DISPLAY() 0
#endif
