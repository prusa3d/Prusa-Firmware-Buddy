/*
 * 	window_scroll_text.c
 */

#include "window_scroll_text.h"
#include "gui.h"
#include "stm32f4xx_hal.h"
#include "display.h"
//#include "display_helper.h"

void window_scroll_text_init(window_scroll_text_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = 0;
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
    window->roll.phase = window->roll.setup = window->roll.px_cd = window->roll.count = 0;
}

void window_scroll_text_draw(window_scroll_text_t *window) {

    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {

        /*  render_scroll_text_align(window->win.rect,
            window->text,
            window->font,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text,
            window->text_rect,
            window->roll_flag,
            window->roll_progress,
            window->roll_count,
            window->width_countdown);
        */

        if (window->text == 0) {
            display->fill_rect(window->win.rect, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);
            window->win.flg &= ~WINDOW_FLG_INVALID;
            return;
        }

        if (window->roll.setup == 0) {
            window->roll.rect = roll_text_rect_meas(window->win.rect, window->text, window->font, window->padding, window->alignment);
            window->roll.count = text_rolls_meas(window->roll.rect, window->text, window->font);
            window->roll.progress = window->roll.px_cd = 0;
            window->roll.setup = 1;
        }

        const char *str = window->text;
        str += window->roll.progress;

        rect_ui16_t set_txt_rc = window->roll.rect;
        if (window->roll.px_cd != 0) {
            set_txt_rc.x += window->roll.px_cd;
            set_txt_rc.w -= window->roll.px_cd;
        }

        if (set_txt_rc.w && set_txt_rc.h) {
            rect_ui16_t rc_t = { window->win.rect.x, window->win.rect.y, window->win.rect.w, set_txt_rc.y - window->win.rect.y };
            rect_ui16_t rc_b = { window->win.rect.x, set_txt_rc.y + set_txt_rc.h, window->win.rect.w, (window->win.rect.y + window->win.rect.h) - (set_txt_rc.y + set_txt_rc.h) };
            rect_ui16_t rc_l = { window->win.rect.x, window->win.rect.y, set_txt_rc.x - window->win.rect.x, window->win.rect.h };
            rect_ui16_t rc_r = { set_txt_rc.x + set_txt_rc.w, window->win.rect.y, (window->win.rect.x + window->win.rect.w) - (set_txt_rc.x + set_txt_rc.w), window->win.rect.h };
            display->fill_rect(rc_t, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);
            display->fill_rect(rc_b, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);
            display->fill_rect(rc_l, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);
            display->fill_rect(rc_r, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);

            display->draw_text(set_txt_rc, str, window->font, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back,
                (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text);
        } else {
            display->fill_rect(window->win.rect, (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back);
        }

        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

void window_scroll_text_event(window_scroll_text_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_TIMER) {

        switch (window->roll.phase) {
        case ROLL_SETUP:
            gui_timers_delete_by_window_id(window->win.id);
            gui_timer_create_periodical(TEXT_ROLL_DELAY_MS, window->win.id);
            if (window->roll.setup == 1)
                window->roll.phase = ROLL_GO;
            window_invalidate(window->win.id);
            break;
        case ROLL_GO:
            if (window->roll.count > 0 || window->roll.px_cd > 0) {
                if (window->roll.px_cd == 0) {
                    window->roll.px_cd = window->font->w;
                    window->roll.count--;
                    window->roll.progress++;
                }
                window->roll.px_cd--;
                window_invalidate(window->win.id);
            } else {
                window->roll.phase = ROLL_STOP;
            }
            break;
        case ROLL_STOP:
            gui_timers_delete_by_window_id(window->win.id);
            gui_timer_create_periodical(TEXT_ROLL_INITIAL_DELAY_MS, window->win.id);
            window->roll.phase = ROLL_RESTART;
            window_invalidate(window->win.id);
            break;
        case ROLL_RESTART:
            window->roll.setup = 0;
            window->roll.phase = ROLL_SETUP;
            window_invalidate(window->win.id);
            break;
        }
    }
}

void window_scroll_text_done(window_scroll_text_t *window) {
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
