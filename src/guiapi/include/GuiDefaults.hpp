// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"

struct GuiDefaults_t {
    color_t ColorBack;
    color_t ColorText;
    color_t ColorDisabled;
    padding_ui8_t Padding;
    uint8_t Alignment;                //todo enum
    rect_ui16_t RectHeader;           // header location & size
    rect_ui16_t RectScreenBody;       // screen body location & size
    rect_ui16_t RectScreenBodyNoFoot; // screen body without footer location & size
    rect_ui16_t RectScreen;           // full screen body without footer & header location & size
    rect_ui16_t RectFooter;           // footer location & size
    uint8_t ButtonHeight;
    uint8_t ButtonSpacing;
    uint8_t FrameWidth; //frame width, what is this? it is used in buttons
    static font_t *Font;
    static font_t *FontBig;
};

static constexpr GuiDefaults_t GuiDefaults = {
    COLOR_BLACK,                //ColorBack
    COLOR_WHITE,                //ColorText
    COLOR_SILVER,               //ColorDisabled
    { 2, 2, 2, 2 },             //Padding
    ALIGN_LEFT_TOP,             //todo enum
    { 0, 0, 240, 32 - 0 },      // default header location & size
    { 0, 32, 240, 267 - 32 },   // default screen body location & size
    { 0, 32, 240, 320 - 32 },   // screen body without footer location & size
    { 0, 0, 240, 320 },         // full screen body without footer & header location & size
    { 0, 267, 240, 320 - 267 }, // default footer location & size
    30,                         // default button height
    6,                          // default button spacing
    10                          // default frame width, what is this? it is used in buttons
};
