/*
 * window_dlg_wait.c
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */
#include "window_dlg_wait.hpp"
#include "gui.hpp"
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

#define ANIMATION_MILISEC_DELAY 500 // number of milisecond for frame change

window_dlg_wait_t::window_dlg_wait_t(rect_ui16_t rect)
    : IDialog(rect)
    , text(this, { rect.x, uint16_t(rect.y + 10), rect.w, uint16_t(30) }, is_closed_on_click_t::no, _("Please wait"))
    , animation(this, { uint16_t(rect.x + 110), uint16_t(rect.y + 50) }) {
    text.font = GuiDefaults::FontBig;
    text.SetAlignment(ALIGN_CENTER);
}

/* if (window->components & DLG_W8_DRAW_FRAME) { // grey frame enabled
            const uint16_t w = display::GetW();
            const uint16_t h = display::GetH();

            display::DrawLine(point_ui16(rc.x, rc.y), point_ui16(w - 1, rc.y), COLOR_GRAY);
            display::DrawLine(point_ui16(rc.x, rc.y), point_ui16(rc.x, h - 67), COLOR_GRAY);
            display::DrawLine(point_ui16(w - 1, rc.y), point_ui16(w - 1, h - 67), COLOR_GRAY);
            display::DrawLine(point_ui16(rc.x, h - 67), point_ui16(w - 1, h - 67), COLOR_GRAY);
        }*/

/*rect_ui16_t icon_rc = rc;
            icon_rc.h = icon_rc.w = 30;
            icon_rc.x += 110;
            icon_rc.y += 50;
            render_icon_align(icon_rc, IDR_PNG_wizard_icon_hourglass, COLOR_BLACK, ALIGN_CENTER);
*/

void gui_dlg_wait(void (*closing_callback)()) {

    window_dlg_wait_t dlg(GuiDefaults::RectScreenBody);
    dlg.MakeBlocking(closing_callback);
}
