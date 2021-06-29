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

window_dlg_wait_t::window_dlg_wait_t(Rect16 rect)
    : IDialog(rect)
    , text(this, { rect.Left(), int16_t(rect.Top() + 10), rect.Width(), uint16_t(30) }, is_multiline::no, is_closed_on_click_t::no, _("Please wait"))
    , animation(this, { int16_t(rect.Left() + 110), int16_t(rect.Top() + 50) }) {
    text.font = GuiDefaults::FontBig;
    text.SetAlignment(Align_t::Center());
}

void gui_dlg_wait(std::function<void()> closing_callback) {

    window_dlg_wait_t dlg(GuiDefaults::RectScreenBody);
    dlg.MakeBlocking(closing_callback);
}
