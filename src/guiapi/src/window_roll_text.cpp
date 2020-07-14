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
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = string_view_utf8::MakeNULLSTR();
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
    window->roll.count = window->roll.px_cd = window->roll.progress = 0;
    window->roll.phase = ROLL_SETUP;
    window->roll.setup = TXTROLL_SETUP_INIT;
    gui_timer_create_txtroll(TEXT_ROLL_INITIAL_DELAY_MS, window->id);
}

void window_roll_text_draw(window_roll_text_t *window) {

    if (((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {

        render_roll_text_align(window->rect,
            window->text,
            window->font,
            window->padding,
            window->alignment,
            (window->flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back,
            (window->flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text,
            &window->roll);

        window->flg &= ~WINDOW_FLG_INVALID;
    }
}

void window_roll_text_event(window_roll_text_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_TIMER) {
        roll_text_phasing(window->id, window->font, &window->roll);
    }
}

void window_roll_text_done(window_roll_text_t *window) {
    gui_timers_delete_by_window_id(window->id);
}

const window_class_roll_text_t window_class_roll_text = {
    {
        WINDOW_CLS_ROLL_TEXT,
        sizeof(window_roll_text_t),
        (window_init_t *)window_roll_text_init,
        (window_done_t *)window_roll_text_done,
        (window_draw_t *)window_roll_text_draw,
        (window_event_t *)window_roll_text_event,
    },
};
