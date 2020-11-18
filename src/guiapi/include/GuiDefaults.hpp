// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"
#include "Rect16.h"

struct GuiDefaults {
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr color_t ColorSelected = COLOR_ORANGE;
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

    //special setting for menu items
    static constexpr bool MenuLinesBetweenItems = false;
    static constexpr color_t MenuColorBack = ColorBack;
    static constexpr color_t MenuColorText = ColorText;
    static constexpr color_t MenuColorDisabled = ColorDisabled;
    static font_t *FontMenuItems;                        // for menu items
    static font_t *FontMenuSpecial;                      // for units in menu
    static constexpr bool MenuSwitchHasBrackets = false; // draw brackets around switch values in menus
    static constexpr bool MenuSpinHasUnits = false;      // draw units behind spin
    static constexpr bool MenuHasScrollbar = false;
    static constexpr size_t MenuUseFixedUnitWidth = 28; // 0 == calculate in runtime
    static constexpr size_t MenuScrollbarWidth = 2;
    static constexpr size_t MenuItemDelimiterPadding = 6;
    static constexpr size_t MenuItemDelimeterHeight = 1;
    static constexpr padding_ui8_t MenuPadding = padding_ui8_t({ 6, 6, 6, 6 });
    static constexpr padding_ui8_t MenuPaddingSpecial = padding_ui8_t({ 0, 6, 0, 0 });

    static constexpr uint8_t MenuAlignment = ALIGN_LEFT_TOP;
    static constexpr size_t MenuIconWidth = 25;
};
