/**
 * @file screen_print_preview_base.cpp
 */

#include "screen_print_preview_base.hpp"
#include "gcode_description.hpp" // TITLE_HEIGHT

ScreenPrintPreviewBase::ScreenPrintPreviewBase()
    : title_text(this, Rect16(PADDING, PADDING, display::GetW() - 2 * PADDING, TITLE_HEIGHT))
    , radio(this, GuiDefaults::GetIconnedButtonRect(GetRect())) {
    radio.SetHasIcon();
    radio.SetBlackLayout(); // non iconned buttons have orange background
    radio.SetBtnCount(2);
}
