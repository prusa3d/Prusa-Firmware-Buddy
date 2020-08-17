/*  window_roll_text.c
*   \brief used in texts that are too long for standart display width
*
*  Created on: May 6, 2020
*      Author: Migi - michal.rudolf<at>prusa3d.cz
*/

#include "window_roll_text.hpp"
#include "gui_timer.h"
#include "gui.hpp"
#include "stm32f4xx_hal.h"
#include "display.h"

void window_roll_text_init(window_roll_text_t *window) {
    window->color_back = GuiDefaults::ColorBack;
    window->color_text = GuiDefaults::ColorText;
    window->font = GuiDefaults::Font;
    window->text = string_view_utf8::MakeNULLSTR();
    window->padding = GuiDefaults::Padding;
    window->alignment = GuiDefaults::Alignment;
    window->roll.count = window->roll.px_cd = window->roll.progress = 0;
    window->roll.phase = ROLL_SETUP;
    window->roll.setup = TXTROLL_SETUP_INIT;
    gui_timer_create_txtroll(window, TEXT_ROLL_INITIAL_DELAY_MS);
}

void window_roll_text_draw(window_roll_text_t *window) {

    if (window->IsInvalid() && window->IsVisible()) {

        render_roll_text_align(window->rect,
            window->text,
            window->font,
            window->padding,
            window->alignment,
            (window->IsFocused()) ? window->color_text : window->color_back,
            (window->IsFocused()) ? window->color_back : window->color_text,
            &window->roll);

        window->Validate();
        ;
    }
}

void window_roll_text_event(window_roll_text_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_TIMER) {
        roll_text_phasing(window, window->font, &window->roll);
    }
}

void window_roll_text_done(window_roll_text_t *window) {
    gui_timers_delete_by_window(window);
}

window_roll_text_t::window_roll_text_t(window_t *parent, Rect16 rect)
    : window_text_t(parent, rect) {
    roll.count = roll.px_cd = roll.progress = 0;
    roll.phase = ROLL_SETUP;
    roll.setup = TXTROLL_SETUP_INIT;
    gui_timer_create_txtroll(this, TEXT_ROLL_INITIAL_DELAY_MS);
}
