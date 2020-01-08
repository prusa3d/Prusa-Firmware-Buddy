/*
 * 	window_scroll_text.c
 */

#include "window_scroll_text.h"
#include "gui_timer.h"
#include "gui.h"
#include "stm32f4xx_hal.h"
#include "display.h"

void window_scroll_text_init(window_scroll_text_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = 0;
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
    window->roll.phase = window->roll.setup = window->roll.px_cd = window->roll.count = 0;
    gui_timer_create_txtroll(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
}

void window_scroll_text_draw(window_scroll_text_t *window) {

    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {

        render_scroll_text_align( window->win.rect,
            window->text,
            window->font,
            window->padding,
            window->alignment,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text,
            &window->roll);

        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

void window_scroll_text_event(window_scroll_text_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_TIMER) {
        scroll_text_phasing(window->win.id, window->font, &window->roll);
    }
}

void window_scroll_text_done(window_scroll_text_t *window) {
    gui_timers_delete_by_window_id(window->win.id);
}

const window_class_scroll_text_t window_class_scroll_text = {
    {
        WINDOW_CLS_SCROLL_TEXT,
        sizeof(window_scroll_text_t),
        (window_init_t *)window_scroll_text_init,
        (window_done_t *)window_scroll_text_done,
        (window_draw_t *)window_scroll_text_draw,
        (window_event_t *)window_scroll_text_event,
    },
};
