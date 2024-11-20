/*
 * window_dlg_wait.c
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */
#include "window_dlg_wait.hpp"
#include "gui.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

static const constexpr uint16_t ANIMATION_MILISEC_DELAY = 500; // number of milisecond for frame change
static const constexpr int animation_y = GuiDefaults::EnableDialogBigLayout ? 120 : 130; // animation anchor point on Y axis
static const constexpr int animation_x = GuiDefaults::EnableDialogBigLayout ? 220 : 110; // animation anchor point on X axis
static const constexpr int text_y_offset = GuiDefaults::EnableDialogBigLayout ? 30 : 10; // text point on y axis
static const constexpr int second_text_y_offset = GuiDefaults::EnableDialogBigLayout ? 67 : 45; // text point on y axis

window_dlg_wait_t::window_dlg_wait_t(Rect16 rect, const string_view_utf8 &second_text_string)
    : IDialogMarlin(rect)
    , text(this, { rect.Left(), int16_t(rect.Top() + text_y_offset), rect.Width(), uint16_t(30) }, is_multiline::no, is_closed_on_click_t::no, _("Please wait"))
    , second_text(this, { int16_t(rect.Left() + GuiDefaults::FramePadding), int16_t(rect.Top() + second_text_y_offset), uint16_t(rect.Width() - 2 * GuiDefaults::FramePadding), uint16_t(60) }, is_multiline::yes, is_closed_on_click_t::no, second_text_string)
    , animation(this, { int16_t(rect.Left() + animation_x), int16_t(rect.Top() + animation_y) }) {
    text.set_font(GuiDefaults::FontBig);
    text.SetAlignment(Align_t::Center());

    second_text.set_font(GuiDefaults::FooterFont);
    second_text.SetAlignment(Align_t::Center());
}

void gui_dlg_wait(stdext::inplace_function<void()> closing_callback, const string_view_utf8 &second_string) {
    window_dlg_wait_t dlg(second_string);
    Screens::Access()->gui_loop_until_dialog_closed(closing_callback);
}
