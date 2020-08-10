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

#define ANIMATION_MILISEC_DELAY 500 // number of milisecond for frame change

window_dlg_wait_t::window_dlg_wait_t(rect_ui16_t rect)
    : IDialog(rect)
    , text(this, { rect.x, uint16_t(rect.y + 10), rect.w, uint16_t(30) }, is_closed_on_click_t::no, _("Please wait"))
    , animation(this, { uint16_t(rect.x + 110), uint16_t(rect.y + 50) }) {
    text.font = GuiDefaults::FontBig;
    text.SetAlignment(ALIGN_CENTER);
}

void gui_dlg_wait(void (*closing_callback)()) {

    window_dlg_wait_t dlg(GuiDefaults::RectScreenBody);
    dlg.MakeBlocking(closing_callback);
}
