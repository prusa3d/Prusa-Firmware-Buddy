// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"

struct GuiDefaults {
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr padding_ui8_t Padding = { 2, 2, 2, 2 };
    static constexpr uint8_t Alignment = ALIGN_LEFT_TOP;                          //todo enum
    static constexpr rect_ui16_t RectHeader = { 0, 0, 240, 32 - 0 };              // default header location & size
    static constexpr rect_ui16_t RectScreenBody = { 0, 32, 240, 267 - 32 };       // default screen body location & size
    static constexpr rect_ui16_t RectScreenBodyNoFoot = { 0, 32, 240, 320 - 32 }; // screen body without footer location & size
    static constexpr rect_ui16_t RectScreen = { 0, 0, 240, 320 };                 // full screen body without footer & header location & size
    static constexpr rect_ui16_t RectFooter = { 0, 267, 240, 320 - 267 };         // default footer location & size
    static constexpr uint8_t ButtonHeight = 30;                                   // default button height
    static constexpr uint8_t ButtonSpacing = 6;                                   // default button spacing
    static constexpr uint8_t FrameWidth = 10;                                     // default frame width, what is this? it is used in buttons
    static font_t *Font;                                                          //todo constexpr
    static font_t *FontBig;                                                       //todo constexpr
};
