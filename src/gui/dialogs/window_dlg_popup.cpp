/*
 * window_dlg_popup.c
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.h"
#include "display_helper.h"
#include "gui.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "../lang/i18n.h"

#define POPUP_DELAY_MS 1000

int16_t WINDOW_CLS_DLG_POPUP = 0;

extern msg_stack_t msg_stack;

void window_dlg_popup_init(window_dlg_popup_t *window) {
    window->win.flg |= WINDOW_FLG_ENABLED;
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;
}

void window_dlg_popup_draw(window_dlg_popup_t *window) {
    if (window->win.f_visible) {
        rect_ui16_t rc = window->win.rect;
        rc.h = 140;

        if (window->win.f_invalid) {
            display::FillRect(rc, window->color_back);
            rect_ui16_t text_rc = rc;
            text_rc.x += 10;
            text_rc.y += 20;
            text_rc.h = 30;
            text_rc.w -= 10;
            render_text_align(text_rc, _(window->text),
                window->font, window->color_back,
                window->color_text, window->padding,
                ALIGN_LEFT_CENTER);
            window->win.f_invalid = 0;
        }
    }
}

const window_class_dlg_popup_t window_class_dlg_popup = {
    {
        WINDOW_CLS_USER,
        sizeof(window_dlg_popup_t),
        (window_init_t *)window_dlg_popup_init,
        0,
        (window_init_t *)window_dlg_popup_draw,
        0,
    },
};

void gui_pop_up(void) {

    static uint8_t opened = 0;
    if (opened == 1)
        return;
    opened = 1;

    window_dlg_popup_t dlg;

    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_DLG_POPUP, 0, rect_ui16(0, 32, 240, 120), &dlg);
    memset(dlg.text, '\0', sizeof(dlg.text) * sizeof(char)); // set to zeros to be on the safe side
    strlcpy(dlg.text, msg_stack.msg_data[0], sizeof(dlg.text));
    window_popup_ptr = (window_t *)&dlg;
    gui_invalidate();
    window_set_capture(id);

    dlg.timer = HAL_GetTick();

    while ((HAL_GetTick() - dlg.timer) < POPUP_DELAY_MS) {
        gui_loop();
    }

    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
    opened = 0;
}
