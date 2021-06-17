// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"
#include "Rect16.h"
#include "align.hpp"
#include "footer_def.hpp"

struct GuiDefaults {
    // Footer settings
    static constexpr size_t FooterLines = FOOTER_LINES__;
    static constexpr size_t FooterIconTextSpace = 3;                        //space between icon and text of footer item in px
    static constexpr padding_ui8_t FooterPadding = { 4, 4, 4, 4 };          //number of edge pixels that will remain black in all cases
    static constexpr size_t FooterLinesSpace = 8;                           //space between footer lines
    static constexpr Rect16::Height_t FooterItemHeight = 16;                //must match font and icon height
    static constexpr size_ui16_t FooterIconSize = { 16, FooterItemHeight }; //DO NOT CHANGE HEIGHT!!! it must match item height (item height can be changed instead), real icon height can be smaller
    static constexpr Rect16::Height_t FooterTextHeight = FooterItemHeight;  //DO NOT CHANGE!!!        it must match item height (item height can be changed instead), real text height can be smaller
    static font_t *FooterFont;                                              //TODO constexpr, font_9x16, IT MUST MATCH OR BE SMALLER THAN FooterItemHeight!!!

    //display specific defaults
    //TODO bind this values
    static constexpr size_t ScreenWidth = 240;
    static constexpr size_t ScreenHeight = 320;
    static constexpr size_t FooterHeight = FooterLines * FooterItemHeight + (FooterLines - 1) * FooterLinesSpace + FooterPadding.top + FooterPadding.bottom;
    static constexpr size_t HeaderHeight = 32;

    // COMMON DEFAULTS

    // Color settings
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr color_t ColorSelected = COLOR_ORANGE;
    static constexpr color_t COLOR_VALUE_VALID = COLOR_WHITE;
    static constexpr color_t COLOR_VALUE_INVALID = COLOR_WHITE; //COLOR_YELLOW
    // Menu color settings
    static constexpr color_t MenuColorBack = ColorBack;
    static constexpr color_t MenuColorText = ColorText;
    static constexpr color_t MenuColorDisabled = ColorDisabled;

    // Text settings
    static constexpr padding_ui8_t Padding = { 2, 2, 2, 2 };
    static constexpr Align_t Align() { return Align_t::LeftTop(); }
    static font_t *Font;    //todo constexpr
    static font_t *FontBig; //todo constexpr

    // Layout settings
    static constexpr size_t BodyHeight = ScreenHeight - FooterHeight - HeaderHeight;
    static constexpr Rect16 RectHeader = { 0, 0, ScreenWidth, HeaderHeight };                                   // default header location & size
    static constexpr Rect16 RectScreenBody = { 0, HeaderHeight, ScreenWidth, BodyHeight };                      // default screen body location & size
    static constexpr Rect16 RectScreen = { 0, 0, ScreenWidth, ScreenHeight };                                   // full screen body & header
    static constexpr Rect16 RectScreenNoFoot = { 0, 0, ScreenWidth, ScreenHeight - FooterHeight };              // screen body without footer location & size
    static constexpr Rect16 RectScreenNoHeader = { 0, HeaderHeight, ScreenWidth, ScreenHeight - HeaderHeight }; // screen body without header location & size
    static constexpr Rect16 RectFooter = { 0, ScreenHeight - FooterHeight, ScreenWidth, FooterHeight };         // default footer location & size
    static constexpr Rect16 PreviewThumbnailRect = { 10, HeaderHeight + 12, 220, 124 };                         // rect describing preview thumbnail
    static constexpr uint8_t ButtonHeight = 30;                                                                 // default button height
    static constexpr uint8_t ButtonSpacing = 6;                                                                 // default button spacing
    static constexpr uint8_t FrameWidth = 10;                                                                   // default frame padding
    static const uint32_t MAX_DIALOG_BUTTON_COUNT = 4;                                                          // maximum number of radio buttons

    // Menu settings
    static constexpr size_t MenuIconWidth = 25;

    // Menu text settings
    static font_t *FontMenuItems;   // for menu items
    static font_t *FontMenuSpecial; // for units in menu
    static constexpr Align_t MenuAlignment() { return Align_t::LeftTop(); }
    static constexpr padding_ui8_t MenuPadding = padding_ui8_t({ 6, 6, 6, 6 });
    static constexpr padding_ui8_t MenuPaddingSpecial = padding_ui8_t({ 0, 6, 0, 0 });

    // Enable new menu features
    static constexpr bool MenuLinesBetweenItems = false;
    static constexpr bool MenuSwitchHasBrackets = false; // draw brackets around switch values in menus
    static constexpr bool MenuSpinHasUnits = false;      // draw units behind spin
    static constexpr bool MenuHasScrollbar = false;

    // New menu feature settings
    static constexpr size_t MenuUseFixedUnitWidth = 28; // 0 == calculate in runtime
    static constexpr size_t MenuScrollbarWidth = 2;
    static constexpr padding_ui8_t MenuItemDelimiterPadding = padding_ui8_t({ 6, 0, 6, 0 });
    static constexpr size_t MenuItemDelimeterHeight = MenuLinesBetweenItems ? 1 : 0;
};
