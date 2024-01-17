// guiconfig.h - guiapi configuration file
#pragma once

#include <inttypes.h>
#include <device/board.h>

///////////////////////////
// display type selector //
///////////////////////////
#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    #define USE_ILI9488
    #define MENU_HAS_SCROLLBAR true
#elif BOARD_IS_BUDDY
    #define USE_ST7789
    #define MENU_HAS_SCROLLBAR false
#else
    #define USE_MOCK_DISPLAY
    #define MENU_HAS_SCROLLBAR false
#endif

//--------------------------------------
// FreeRTOS Signals

static const uint32_t MENU_TIMEOUT_MS = 30000;
