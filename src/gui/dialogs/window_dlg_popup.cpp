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
#include "i18n.h"
#include "ScreenHandler.hpp"

#define POPUP_DELAY_MS 1000

extern msg_stack_t msg_stack;

window_dlg_popup_t::window_dlg_popup_t(window_t *parent, Rect16 rect)
    : window_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , font_title(GuiDefaults::FontBig)
    , padding(GuiDefaults::Padding)
    , timer(0)
    , text("") {
    Enable();
}
/*
void window_dlg_popup_draw(window_dlg_popup_t *window) {
    if (window->IsVisible()) {
        Rect16 rc = window->rect;
        rc = Rect16::Height_t(140);

        if (window->IsInvalid()) {
            display::FillRect(rc, window->color_back);
            Rect16 text_rc = rc;
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
}*/

void gui_pop_up(void) {

    static uint8_t opened = 0;
    if (opened == 1)
        return;
    opened = 1;

    window_dlg_popup_t dlg(nullptr, Rect16(0, 32, 240, 120));

    window_t *id_capture = window_t::GetCapturedWindow();
    memset(dlg.text, '\0', sizeof(dlg.text) * sizeof(char)); // set to zeros to be on the safe side
    strlcpy(dlg.text, msg_stack.msg_data[0], sizeof(dlg.text));
    gui_invalidate();
    dlg.SetCapture();

    dlg.timer = HAL_GetTick();

    while ((HAL_GetTick() - dlg.timer) < POPUP_DELAY_MS) {
        gui_loop();
    }

    //window_destroy(id);
    if (id_capture)
        id_capture->SetCapture();
    window_t *pWin = Screens::Access()->Get();
    if (pWin != 0)
        pWin->Invalidate();
    opened = 0;
}
