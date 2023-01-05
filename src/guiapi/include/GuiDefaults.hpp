// GuiDefaults.hpp
#pragma once
#include "guitypes.hpp"
#include "Rect16.h"
#include "display.h"
#include "guiconfig.h"
#include "align.hpp"
#include "footer_def.hpp"
#include "color_scheme.hpp"
#include "window_types.hpp"
#include <optional>
#include "option/development_items.h"

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
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    //TODO bind this values
    static constexpr size_t ScreenWidth = 240;
    static constexpr size_t ScreenHeight = 320;
    static constexpr size_t FooterHeight = FooterLines * FooterItemHeight + (FooterLines - 1) * FooterLinesSpace + FooterPadding.top + FooterPadding.bottom;
    static constexpr size_t HeaderHeight = 32;
    static constexpr Rect16 PreviewThumbnailRect = { 10, HeaderHeight + 12, 220, 124 };
    static constexpr Rect16 ProgressThumbnailRect = { 0, 0, 200, 240 };
    static constexpr uint8_t ButtonHeight = 30; // default button height
    static constexpr uint8_t ButtonSpacing = 6; // default button spacing

#endif // USE_<display>

    // COMMON DEFAULTS
    static constexpr size_t infoDefaultLen = ScreenWidth > 240 ? 22 : 10; // null included, mac address must fit - need to be at least 18

    // Color settings
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr color_t ColorSelected = COLOR_ORANGE;
    static constexpr color_t COLOR_VALUE_VALID = COLOR_WHITE;
    static constexpr color_t COLOR_VALUE_INVALID = COLOR_WHITE; //COLOR_YELLOW
    static constexpr color_scheme ClickableIconColorScheme = { ScreenWidth > 240 ? COLOR_DARK_GRAY : COLOR_BLACK, COLOR_WHITE, ColorBack, ColorDisabled };
    // Menu color settings
    static constexpr color_t MenuColorBack = ColorBack;
    static constexpr color_t MenuColorFocusedBack = COLOR_WHITE;
    static constexpr color_t MenuColorText = ColorText;
    static constexpr color_t MenuColorDisabled = ColorDisabled;
    static constexpr color_t MenuColorDevelopment = COLOR_LIGHT_GREEN;
    static constexpr color_t MenuColorDevelopmentDisabled = COLOR_DARK_GREEN;

    // Text settings
    // TODO fix unit tests, so this is not needed
#if defined(USE_MOCK_DISPLAY)
    static constexpr padding_ui8_t Padding = { 2, 2, 2, 2 };
#else
    static constexpr padding_ui8_t Padding = { 0, 0, 0, 0 };
#endif // USE_<display>
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

    static constexpr Rect16 GetButtonRect(Rect16 rc_frame) { return Rect16(rc_frame.Left() + ButtonSpacing,
        rc_frame.Top() + (rc_frame.Height() - ButtonHeight - FrameWidth), rc_frame.Width() - 2 * ButtonSpacing, ButtonHeight); }
    static constexpr Rect16 GetDialogRect(std::optional<has_footer> footer) {
        if (!footer) {
            return DialogFrameRect;
        }
        if ((*footer) == has_footer::no)
            return RectScreenBody;
        // has_footer::yes
        return RectScreenNoHeader;
    }
    static constexpr Rect16 GetIconnedButtonRect(Rect16 rc_frame) { return Rect16(rc_frame.Left() + ButtonSpacing,
        227, rc_frame.Width() - 2 * ButtonSpacing, 70 + 22); } //TODO calculate
    static constexpr Rect16 GetButtonRect_AvoidFooter(Rect16 rc_frame) { return GetButtonRect(rc_frame - Rect16::Height_t(FooterHeight)); }

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr uint8_t FrameWidth = 10;          // default frame padding
#endif                                                 // USE_<display>
    static const uint32_t MAX_DIALOG_BUTTON_COUNT = 4; // maximum number of radio buttons

    // Menu settings
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr EFooter MenuFooter = EFooter::On; // Menu has footer or not
#else
    static constexpr EFooter MenuFooter = EFooter::Off; // Menu has footer or not
#endif
    static constexpr size_t MenuIconWidth = 25;

    // Menu text settings
    static font_t *FontMenuItems;   // for menu items
    static font_t *FontMenuSpecial; // for units in menu
    static constexpr Align_t MenuAlignment() { return Align_t::LeftTop(); }

    // Enable new menu features
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr bool MenuLinesBetweenItems = false;
    static constexpr bool MenuSwitchHasBrackets = false;                                       // draw brackets around switch values in menus
    static constexpr bool MenuSpinHasUnits = false;                                            // draw units behind spin
#endif                                                                                         // USE_<display>
    static constexpr bool ShowDevelopmentTools = static_cast<bool>(option::development_items); // Show menu items for development

    // New menu feature settings
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr size_t MenuUseFixedUnitWidth = 28; // 0 == calculate in runtime
    static constexpr Rect16::Width_t MenuScrollbarWidth = MENU_HAS_SCROLLBAR ? 2 : 0;
    static constexpr uint8_t MenuItemCornerRadius = 0;
    static constexpr padding_ui8_t MenuItemDelimiterPadding = padding_ui8_t({ 6, 0, 6, 0 });
    static constexpr padding_ui8_t MenuPaddingItems = padding_ui8_t({ 6, 6, 6, 6 });
    static constexpr padding_ui8_t MenuPaddingSpecial = padding_ui8_t({ 0, 6, 0, 0 });
#endif

    static constexpr padding_ui8_t MenuPadding = padding_ui8_t({ 14, 0, 5, 0 });
    static constexpr size_t MenuItemDelimeterHeight = MenuLinesBetweenItems ? 1 : 0;

    static constexpr Rect16::Width_t MenuIcon_w = MENU_HAS_BUTTONS ? 39 : 0;  // 64;
    static constexpr Rect16::Height_t MenuIcon_h = MENU_HAS_BUTTONS ? 39 : 0; // 64;

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr padding_ui8_t FileBrowserPadding = padding_ui8_t({ 0, 0, 0, 0 });
#endif
    static constexpr Rect16 FileBrowserRect = Rect16::CutPadding(RectScreenNoHeader, FileBrowserPadding);

    // New msgbox
    static constexpr Rect16 MsgBoxLayoutRect = { 70, 90, 363, 120 };                                                                                 // Msgbox rect for drawing icon + text
    static constexpr Rect16 MessageTextRect = Rect16(GuiDefaults::MsgBoxLayoutRect.Left() + 48 + 15, GuiDefaults::MsgBoxLayoutRect.Top(), 300, 120); // 48px icon + 10px icon-text delimeter
    static constexpr Rect16 MessageIconRect = Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), GuiDefaults::MsgBoxLayoutRect.Top(), 48, 48);
    static constexpr uint8_t DefaultCornerRadius = 8;
    static constexpr uint8_t IconButtonSize = 64;
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr Rect16 DialogFrameRect = RectScreenBody;
    static constexpr uint16_t RadioButtonCornerRadius = 0;
    static constexpr bool EnableDialogBigLayout = false;
#endif // USE_<DISPLAY>
};
