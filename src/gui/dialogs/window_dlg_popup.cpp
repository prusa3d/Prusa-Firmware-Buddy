/*
 * window_dlg_popup.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.hpp"
#include "display_helper.h"
#include "i18n.h"
#include "ScreenHandler.hpp"

window_dlg_popup_t::window_dlg_popup_t(Rect16 rect, string_view_utf8 txt)
    : IDialog(rect)
    , text(this, rect, is_multiline::yes, is_closed_on_click_t::no, txt) {
    text.SetAlignment(ALIGN_LEFT_TOP);
    text.SetPadding({ 0, 2, 0, 2 });
}

void gui_pop_up(string_view_utf8 txt, uint32_t time) {
    window_dlg_popup_t dlg(Rect16(0, 32, 240, 120), txt);
    const uint32_t open_time = HAL_GetTick();
    // create lambda function, convert it to std::function<void(uint32_t, uint32_t)>
    // and pass it to dlg.MakeBlocking
    dlg.MakeBlocking(
        (std::function<void(uint32_t, uint32_t)>)[](uint32_t open_time_, uint32_t time_) {
            if ((int(HAL_GetTick()) - int(open_time_)) > int(time_))
                Screens::Access()->Close();
        },
        open_time, time);
}
