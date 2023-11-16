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
    if (flags.color_scheme_background || flags.color_scheme_foreground) {
        // TODO keep only following 3 lines in function body, remove rest
        super::unconditionalDraw();
        roll.RenderTextAlign(GetRect(), text, get_font(),
            GetBackColor(), GetTextColor(), padding, GetAlignment());
    } else {
        roll.RenderTextAlign(GetRect(), text, get_font(),
            (IsFocused()) ? GetTextColor() : GetBackColor(),
            (IsFocused()) ? GetBackColor() : GetTextColor(),
            padding, GetAlignment());
    }
}

void window_roll_text_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::TEXT_ROLL) {
        if (roll.Tick() == invalidate_t::yes) {
            Invalidate();
        }
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

window_roll_text_t::window_roll_text_t(window_t *parent, Rect16 rect, string_view_utf8 txt, Align_t align)
    : AddSuperWindow<window_text_t>(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt) {
    this->SetAlignment(align);
    rollInit();
}

void window_roll_text_t::SetText(string_view_utf8 txt) {
    super::SetText(txt);
    rollInit();
}

bool window_roll_text_t::SetRect(Rect16 rect) {
    if (GetRect() == rect) {
        return false; // to avoid pointless assignment/reinit
    }

    super::SetRect(rect);
    rollInit();
    return true;
}
