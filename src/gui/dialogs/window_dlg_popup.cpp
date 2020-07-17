/*
 * window_dlg_popup.cpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#include "window_dlg_popup.hpp"
#include "display_helper.h"
#include "gui.hpp"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "../lang/i18n.h"

#define POPUP_DELAY_MS 1000

int16_t WINDOW_CLS_DLG_POPUP = 0;

extern msg_stack_t msg_stack;

void window_dlg_popup_init(window_dlg_popup_t *window) {
    window->Enable();
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;
}

void window_dlg_popup_draw(window_dlg_popup_t *window) {
    if (window->IsVisible()) {
        rect_ui16_t rc = window->rect;
        rc.h = 140;

        if (window->IsInvalid()) {
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
            window->Validate();
        }
    }
}

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
    dlg.SetCapture();

    dlg.timer = HAL_GetTick();

    while ((HAL_GetTick() - dlg.timer) < POPUP_DELAY_MS) {
        gui_loop();
    }

    window_destroy(id);
    if (window_ptr(id_capture))
        window_ptr(id_capture)->SetCapture();
    window_t *pWin = window_ptr(0);
    if (pWin != 0)
        pWin->Invalidate();
    opened = 0;
}
