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
    static constexpr size_t FooterIconTextSpace = 3; // space between icon and text of footer item in px
    static constexpr padding_ui8_t FooterPadding = { 4, 4, 4, 4 }; // number of edge pixels that will remain black in all cases
    static constexpr size_t FooterLinesSpace = 8; // space between footer lines
    static constexpr Rect16::Height_t FooterItemHeight = 16; // must match font and icon height
    static constexpr size_ui16_t FooterIconSize = { 16, FooterItemHeight }; // DO NOT CHANGE HEIGHT!!! it must match item height (item height can be changed instead), real icon height can be smaller
    static constexpr Rect16::Height_t FooterTextHeight = FooterItemHeight; // DO NOT CHANGE!!!        it must match item height (item height can be changed instead), real text height can be smaller
    static font_t *FooterFont; // TODO constexpr, font_9x16, IT MUST MATCH OR BE SMALLER THAN FooterItemHeight!!!

    // display specific defaults
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    // TODO bind this values
    static constexpr size_t ScreenWidth = 240;
    static constexpr size_t ScreenHeight = 320;
    static constexpr size_t FooterHeight = FooterLines * FooterItemHeight + (FooterLines - 1) * FooterLinesSpace + FooterPadding.top + FooterPadding.bottom;
    static constexpr padding_ui8_t HeaderPadding { 4, 4, 4, 4 }; // number of edge pixels that will remain black in all cases
    static constexpr uint8_t HeaderTextExtraPaddingTop { 1 }; // extra padding to be added to the top, needed if font is weird
    static constexpr auto HeaderTextFont { IDR_FNT_SPECIAL };
    static constexpr size_t HeaderItemHeight { 16 };
    static constexpr size_t HeaderHeight { HeaderItemHeight + HeaderPadding.top + HeaderPadding.bottom };
    static constexpr Rect16 PreviewThumbnailRect = { 10, HeaderHeight + 12, 220, 124 };

    static constexpr size_t SlicerProgressImgWidth { 240 }; // Cannot be changed without consulting slicer
    static constexpr size_t OldSlicerProgressImgWidth { 200 }; // Cannot be changed, also supported progress img width
    static constexpr size_t SlicerProgressImgHeight { 240 }; // Cannot be changed without consulting slicer

    static constexpr size_t ProgressTextHeight { 70 };
    static constexpr size_t ProgressTextTopOffset { 10 };
    static constexpr size_t ProgressBarHeight { 10 };

    static constexpr size_t ProgressThumbnailWidth { ScreenWidth };
    static constexpr size_t ProgressThumbnailHeight { ScreenHeight - ProgressTextHeight - ProgressBarHeight };

    static_assert(SlicerProgressImgWidth == ProgressThumbnailWidth);
    static_assert(SlicerProgressImgHeight == ProgressThumbnailHeight);

    static constexpr Rect16 ProgressThumbnailRect = { 0, 0, ProgressThumbnailWidth, ProgressThumbnailHeight };

    static constexpr uint8_t ButtonHeight = 30; // default button height
    static constexpr uint8_t ButtonSpacing = 6; // default button spacing
    static constexpr uint8_t ButtonIconSize = 64;
    static constexpr uint16_t ButtonIconVerticalSpacing = 24;
#elif defined(USE_ILI9488)
    static constexpr size_t ScreenWidth = 480; // Some values are redundant on purpose - It's more convenient for future display's implementation
    static constexpr size_t ScreenHeight = 320;
    static constexpr size_t FooterHeight = 23;
    static constexpr padding_ui8_t HeaderPadding { 14, 12, 14, 4 }; // number of edge pixels that will remain black in all cases
    static constexpr uint8_t HeaderTextExtraPaddingTop { 1 }; // extra padding to be added to the top, needed if font is weird
    static constexpr auto HeaderTextFont { IDR_FNT_SPECIAL };
    static constexpr size_t HeaderItemHeight { 16 };
    static constexpr size_t HeaderHeight { HeaderItemHeight + HeaderPadding.top + HeaderPadding.bottom };
    static constexpr Rect16 PreviewThumbnailRect = { 30, HeaderHeight + 50, 313, 173 };

    static constexpr size_t SlicerProgressImgWidth { 480 }; // Cannot be changed without consulting slicer
    static constexpr size_t OldSlicerProgressImgWidth { 440 }; // Cannot be changed, also supported progress img width
    static constexpr size_t SlicerProgressImgHeight { 240 }; // Cannot be changed without consulting slicer

    static constexpr size_t ProgressTextHeight { 70 };
    static constexpr size_t ProgressTextTopOffset { 10 };
    static constexpr size_t ProgressBarHeight { 10 };

    static constexpr size_t ProgressThumbnailWidth { ScreenWidth };
    static constexpr size_t ProgressThumbnailHeight { ScreenHeight - ProgressTextHeight - ProgressBarHeight };

    static_assert(SlicerProgressImgWidth == ProgressThumbnailWidth);
    static_assert(SlicerProgressImgHeight == ProgressThumbnailHeight);

    static constexpr Rect16 ProgressThumbnailRect = { 0, 0, ProgressThumbnailWidth, ProgressThumbnailHeight };
    static constexpr uint8_t ButtonHeight = 32; // default button height
    static constexpr uint8_t ButtonSpacing = 6; // default button spacing
    static constexpr uint8_t ButtonIconSize = 80;
    static constexpr uint16_t ButtonIconVerticalSpacing = 37;
#endif // USE_<display>

    // COMMON DEFAULTS
    static constexpr size_t infoDefaultLen = ScreenWidth > 240 ? 22 : 10; // null included, mac address must fit - need to be at least 18

    // Color settings
    static constexpr color_t ColorBack = COLOR_BLACK;
    static constexpr color_t ColorText = COLOR_WHITE;
    static constexpr color_t ColorDisabled = COLOR_SILVER;
    static constexpr color_t ColorSelected = COLOR_ORANGE;
    static constexpr color_t COLOR_VALUE_VALID = COLOR_WHITE;
    static constexpr color_t COLOR_VALUE_INVALID = COLOR_DARK_GRAY;
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
    static font_t *Font; // todo constexpr
    static font_t *FontBig; // todo constexpr

    // Layout settings
    static constexpr size_t BodyHeight = ScreenHeight - FooterHeight - HeaderHeight;
    static constexpr Rect16 RectHeader = { 0, 0, ScreenWidth, HeaderHeight }; // default header location & size
    static constexpr Rect16 RectScreenBody = { 0, HeaderHeight, ScreenWidth, BodyHeight }; // default screen body location & size
    static constexpr Rect16 RectScreen = { 0, 0, ScreenWidth, ScreenHeight }; // full screen body & header
    static constexpr Rect16 RectScreenNoFoot = { 0, 0, ScreenWidth, ScreenHeight - FooterHeight }; // screen body without footer location & size
    static constexpr Rect16 RectScreenNoHeader = { 0, HeaderHeight, ScreenWidth, ScreenHeight - HeaderHeight }; // screen body without header location & size
    static constexpr Rect16 RectFooter = { 0, ScreenHeight - FooterHeight, ScreenWidth, FooterHeight }; // default footer location & size

    static constexpr Rect16 GetButtonRect(Rect16 rc_frame) { return Rect16(rc_frame.Left() + ButtonSpacing,
        rc_frame.Top() + (rc_frame.Height() - ButtonHeight - FramePadding), rc_frame.Width() - 2 * ButtonSpacing, ButtonHeight); }
    static constexpr Rect16 GetDialogRect(std::optional<has_footer> footer) {
        if (!footer) {
            return DialogFrameRect;
        }
        if ((*footer) == has_footer::no) {
            return RectScreenBody;
        }
        // has_footer::yes
        return RectScreenNoHeader;
    }
    static constexpr Rect16 GetButtonRect_AvoidFooter(Rect16 rc_frame) { return GetButtonRect(rc_frame - Rect16::Height_t(FooterHeight)); }

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr uint8_t FramePadding = 10; // default frame padding
    static constexpr Rect16::Width_t FrameWidth = ScreenWidth - 2 * FramePadding; // default frame padding
#elif defined(USE_ILI9488)
    static constexpr uint8_t FramePadding = 6; // default frame padding
#endif // USE_<display>
    static const uint32_t MAX_DIALOG_BUTTON_COUNT = 4; // maximum number of radio buttons

    // Menu settings
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr EFooter MenuFooter = EFooter::On; // Menu has footer or not
#else
    static constexpr EFooter MenuFooter = EFooter::Off; // Menu has footer or not
#endif
    static constexpr size_t MenuIconWidth = 25;

    // Menu text settings
    static font_t *FontMenuItems; // for menu items
    static font_t *FontMenuSpecial; // for units in menu
    static constexpr Align_t MenuAlignment() { return Align_t::LeftTop(); }

    // Enable new menu features
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr bool MenuLinesBetweenItems = false;
    static constexpr bool MenuSwitchHasBrackets = false; // draw brackets around switch values in menus
    static constexpr bool MenuSpinHasUnits = false; // draw units behind spin
#elif defined(USE_ILI9488)
    static constexpr bool MenuLinesBetweenItems = true;
    static constexpr bool MenuSwitchHasBrackets = true; // draw brackets around switch values in menus
    static constexpr bool MenuSpinHasUnits = true; // draw units behind spin
#endif // USE_<display>
    static constexpr bool ShowDevelopmentTools = static_cast<bool>(option::development_items); // Show menu items for development

    // New menu feature settings
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr size_t MenuUseFixedUnitWidth = 28; // 0 == calculate in runtime
    static constexpr Rect16::Width_t MenuScrollbarWidth = MENU_HAS_SCROLLBAR ? 2 : 0;
    static constexpr uint8_t MenuItemCornerRadius = 5;
    static constexpr padding_ui8_t MenuItemDelimiterPadding = padding_ui8_t({ 6, 0, 6, 0 });
    static constexpr padding_ui8_t MenuPaddingItems = padding_ui8_t({ 6, 6, 6, 6 });
    static constexpr padding_ui8_t MenuPaddingSpecial = padding_ui8_t({ 0, 6, 0, 0 });
#elif defined(USE_ILI9488)
    static constexpr size_t MenuUseFixedUnitWidth = 0; // 0 == calculate in runtime
    static constexpr Rect16::Width_t MenuScrollbarWidth = MENU_HAS_SCROLLBAR ? 4 : 0;
    static constexpr uint8_t MenuItemCornerRadius = 5; //
    static constexpr padding_ui8_t MenuItemDelimiterPadding = padding_ui8_t({ 41, 0, 37, 0 });
    static constexpr padding_ui8_t MenuPaddingItems = padding_ui8_t({ 6, 10, 6, 10 });
    static constexpr padding_ui8_t MenuPaddingSpecial = padding_ui8_t({ 0, 6, 0, 0 });
#endif

    static constexpr padding_ui8_t MenuPadding = padding_ui8_t({ 5, 0, 5, 0 });
    static constexpr size_t MenuItemDelimeterHeight = MenuLinesBetweenItems ? 1 : 0;

#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr padding_ui8_t FileBrowserPadding = padding_ui8_t({ 0, 0, 0, 0 });
#elif defined(USE_ILI9488)
    static constexpr padding_ui8_t FileBrowserPadding = padding_ui8_t({ 16, 5, 24, 0 });
#endif
    static constexpr Rect16 FileBrowserRect = Rect16::CutPadding(RectScreenNoHeader, FileBrowserPadding);

    // New msgbox
    static constexpr uint8_t DefaultCornerRadius = 8;
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
    static constexpr Rect16 MsgBoxLayoutRect = { 30, 90, 180, 120 }; // Msgbox rect for drawing icon + text
    static constexpr Rect16 MessageTextRect = Rect16(GuiDefaults::MsgBoxLayoutRect.Left() + 48 + 15, GuiDefaults::MsgBoxLayoutRect.Top(), 117, 120); // 48px icon + 10px icon-text delimeter

    static constexpr Rect16 DialogFrameRect = Rect16(RectScreenNoHeader.Left() + 5, RectScreenNoHeader.Top(), RectScreenNoHeader.Width() - 10, RectScreenNoHeader.Height());
    static constexpr uint16_t RadioButtonCornerRadius = 6;
    static constexpr bool EnableDialogBigLayout = false;
#elif defined(USE_ILI9488)
    static constexpr Rect16 MsgBoxLayoutRect = { 70, 90, 363, ScreenHeight - 90 - ButtonHeight }; // Msgbox rect for drawing icon + text
    static constexpr Rect16 MessageTextRect = Rect16(MsgBoxLayoutRect.Left() + 48 + 15, MsgBoxLayoutRect.Top(), 300, MsgBoxLayoutRect.Height()); // 48px icon + 10px icon-text delimeter

    static constexpr Rect16 DialogFrameRect = RectScreenNoHeader;
    static constexpr uint16_t RadioButtonCornerRadius = 8;
    static constexpr bool EnableDialogBigLayout = true;
#endif // USE_<DISPLAY>
    static constexpr Rect16 MessageIconRect = Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), GuiDefaults::MsgBoxLayoutRect.Top(), 48, 48);

    static constexpr Rect16::Width_t InvalidPrinterIconMargin = 6;
    static constexpr Rect16::Height_t InvalidPrinterLineSpacing = 8;
};
