#pragma once

#include <device/board.h>

///////////////////////////
// display type selector //
///////////////////////////
#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    #define USE_ILI9488
#elif BOARD_IS_BUDDY
    #define USE_ST7789
#else
    #define USE_MOCK_DISPLAY
#endif
