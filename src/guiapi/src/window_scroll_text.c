/*
 * 	window_scroll_text.c
 */

#include "window_scroll_text.h"
#include "gui.h"
#include "stm32f4xx_hal.h"
#include "display.h"
#include "display_helper.h"

void window_scroll_text_init(window_scroll_text_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = 0;
    window->width_countdown = 0;
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
    window->roll_progress = window->roll_count = window->roll_flag = 0;
}

void window_scroll_text_draw(window_scroll_text_t *window) {

    if (window->roll_flag == 0) {
        if (window->text != 0) {
            window->text_rect = roll_text_rect_meas(window->win.rect, window->text, window->font, window->padding, window->alignment);
            window->roll_count = text_rolls_meas(window->text_rect, window->text, window->font);
            window->roll_flag = 1;
            window->roll_progress = window->width_countdown = 0;
        }
    }

    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {

        /*       render_scroll_text_align(window->win.rect,
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
            return;
        }

        const char *str = window->text;
        str += window->roll_progress;

        volatile rect_ui16_t set_txt_rc = window->text_rect;
        if (window->width_countdown != 0) {
            set_txt_rc.x += window->width_countdown;
            set_txt_rc.w -= window->width_countdown--;
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
        if (window->roll_count || window->width_countdown) {
            if (window->width_countdown == 0) {
                window->width_countdown = window->font->w - 1;
                window->roll_count--;
                window->roll_progress++;
            }
        } else {
            window->roll_flag = 0;
        }
        window_invalidate(window->win.id);
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
