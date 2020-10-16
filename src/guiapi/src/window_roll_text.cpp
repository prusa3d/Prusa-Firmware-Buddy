/*  window_roll_text.c
*   \brief used in texts that are too long for standart display width
*
*  Created on: May 6, 2020
*      Author: Migi - michal.rudolf<at>prusa3d.cz
*/

#include "window_roll_text.hpp"
#include "gui_timer.h"
#include "stm32f4xx_hal.h"
#include "display.h"

void window_roll_text_t::unconditionalDraw() {

    roll.RenderTextAlign(rect, text, font, padding,
        GetAlignment(),
        (IsFocused()) ? color_text : color_back,
        (IsFocused()) ? color_back : color_text);
}

void window_roll_text_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    int timer_id = int(param);
    if (event == GUI_event_t::TIMER && timer_id == timer) {
        if (!roll.NeedInit()) {
            roll.Phasing(this, font);
        } else {
            rollInit();
        }
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

window_roll_text_t::window_roll_text_t(window_t *parent, Rect16 rect, string_view_utf8 txt)
    : AddSuperWindow<window_text_t>(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt)
    , timer(gui_timer_create_txtroll(this, TEXT_ROLL_INITIAL_DELAY_MS)) {
    roll.Reset(this);
    rollInit();
}

void window_roll_text_t::SetText(string_view_utf8 txt) {
    super::SetText(txt);
    rollInit();
}
