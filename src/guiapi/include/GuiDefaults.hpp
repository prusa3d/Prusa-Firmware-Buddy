// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"
#include "Rect16.h"

struct GuiDefaults {
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr color_t COLOR_VALUE_VALID = COLOR_WHITE;
    static constexpr color_t COLOR_VALUE_INVALID = COLOR_WHITE; //COLOR_YELLOW
    static constexpr padding_ui8_t Padding = { 2, 2, 2, 2 };
    static constexpr uint8_t Alignment = ALIGN_LEFT_TOP;                     //todo enum
    static constexpr Rect16 RectHeader = { 0, 0, 240, 32 - 0 };              // default header location & size
    static constexpr Rect16 RectScreenBody = { 0, 32, 240, 267 - 32 };       // default screen body location & size
    static constexpr Rect16 RectScreenBodyNoFoot = { 0, 32, 240, 320 - 32 }; // screen body without footer location & size
    static constexpr Rect16 RectScreen = { 0, 0, 240, 320 };                 // full screen body without footer & header location & size
    static constexpr Rect16 RectFooter = { 0, 267, 240, 320 - 267 };         // default footer location & size
    static constexpr uint8_t ButtonHeight = 30;                              // default button height
    static constexpr uint8_t ButtonSpacing = 6;                              // default button spacing
    static constexpr uint8_t FrameWidth = 10;                                // default frame padding
    static font_t *Font;                                                     //todo constexpr
    static font_t *FontBig;                                                  //todo constexpr
    static const uint32_t MAX_DIALOG_BUTTON_COUNT = 4;                       // maximum number of radio buttons
};
