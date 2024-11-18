/*  window_roll_text.c
 *   \brief used in texts that are too long for standart display width
 *
 *  Created on: May 6, 2020
 *      Author: Migi - michal.rudolf<at>prusa3d.cz
 */

#include "window_roll_text.hpp"
#include "stm32f4xx_hal.h"

void window_roll_text_t::unconditionalDraw() {
    if (flags.color_scheme_background || flags.color_scheme_foreground) {
        // TODO keep only following 3 lines in function body, remove rest
        window_text_t::unconditionalDraw();
        roll.render_text(GetRect(), text, get_font(),
            GetBackColor(), GetTextColor(), padding, GetAlignment());
    } else {
        roll.render_text(GetRect(), text, get_font(),
            (IsFocused()) ? GetTextColor() : GetBackColor(),
            (IsFocused()) ? GetBackColor() : GetTextColor(),
            padding, GetAlignment());
    }
}

void window_roll_text_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::TEXT_ROLL) {
        if (roll.Tick() == invalidate_t::yes) {
            Invalidate();
        }
    } else {
        window_text_t::windowEvent(sender, event, param);
    }
}

window_roll_text_t::window_roll_text_t(window_t *parent, Rect16 rect, const string_view_utf8 &txt, Align_t align)
    : window_text_t(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt) {
    this->SetAlignment(align);
    rollInit();
}

void window_roll_text_t::SetText(const string_view_utf8 &txt) {
    window_text_t::SetText(txt);
    rollInit();
}

bool window_roll_text_t::SetRect(Rect16 rect) {
    if (GetRect() == rect) {
        return false; // to avoid pointless assignment/reinit
    }

    window_text_t::SetRect(rect);
    rollInit();
    return true;
}
