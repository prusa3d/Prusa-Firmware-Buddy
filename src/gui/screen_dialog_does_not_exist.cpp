/**
 * @file screen_dialog_does_not_exist.cpp
 * @author Radek Vana
 * @date 2021-11-05
 */

#include "screen_dialog_does_not_exist.hpp"
#include <guiconfig/GuiDefaults.hpp>

ScreenDoesNotExist::ScreenDoesNotExist()
    : txt(window_text_t(this, GetRect(), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)"Screen does not exist"))) {

    ClrMenuTimeoutClose(); // don't close on menu timeout
    SetRedLayout();
    txt.SetTextColor(COLOR_WHITE);
    txt.set_font(GuiDefaults::FontBig);
}

ScreenDialogDoesNotExist::ScreenDialogDoesNotExist() {
    txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)"Dialog does not exist"));
    ths = this;
}

ScreenDialogDoesNotExist::~ScreenDialogDoesNotExist() {
    ths = nullptr;
}

// static variables and member functions
ScreenDialogDoesNotExist *ScreenDialogDoesNotExist::ths = nullptr;

ScreenDialogDoesNotExist *ScreenDialogDoesNotExist::GetInstance() { return ths; }
