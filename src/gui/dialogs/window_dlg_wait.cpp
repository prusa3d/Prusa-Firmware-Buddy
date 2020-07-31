/*
 * window_dlg_wait.c
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */
#include "window_dlg_wait.hpp"
#include "display_helper.h"
#include "gui.hpp"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "resource.h"
#include "../lang/i18n.h"
#include "ScreenHandler.hpp"

#define ANIMATION_MILISEC_DELAY 500 // number of milisecond for frame change

typedef enum {
    ANIM_START = 0,
    ANIM_SAND_1,
    ANIM_SAND_2,
    ANIM_SAND_3,
    ANIM_SAND_4,
    ANIM_COUNT,
} MI_ANIMATION;

window_dlg_wait_t::window_dlg_wait_t(window_t *parent, rect_ui16_t rect)
    : window_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , font_title(GuiDefaults::FontBig)
    , padding(GuiDefaults::Padding)
    , progress(0)
    , animation(ANIM_START)
    , animation_chng(true)
    , progress_chng(true) {
    Enable();
}

void window_dlg_wait_draw(window_dlg_wait_t *window) {
    if (!window->IsVisible())
        return;
    const rect_ui16_t rc = window->rect;

    if (window->IsInvalid()) {
        display::FillRect(rc, window->color_back);

        rect_ui16_t rc_tit = rc;
        rc_tit.y += 10;
        rc_tit.h = 30; // 30 pixels for title
        render_text_align(rc_tit, _("Please wait"), window->font_title, window->color_back, window->color_text, window->padding, ALIGN_CENTER);
        window->Validate();

        if (window->components & DLG_W8_DRAW_FRAME) { // grey frame enabled
            const uint16_t w = display::GetW();
            const uint16_t h = display::GetH();

            display::DrawLine(point_ui16(rc.x, rc.y), point_ui16(w - 1, rc.y), COLOR_GRAY);
            display::DrawLine(point_ui16(rc.x, rc.y), point_ui16(rc.x, h - 67), COLOR_GRAY);
            display::DrawLine(point_ui16(w - 1, rc.y), point_ui16(w - 1, h - 67), COLOR_GRAY);
            display::DrawLine(point_ui16(rc.x, h - 67), point_ui16(w - 1, h - 67), COLOR_GRAY);
        }
    }

    if (window->components & DLG_W8_DRAW_HOURGLASS) { // hourglass animation enabled
        if (window->animation_chng) {                 //hourglass animation timeout
            window->animation_chng = false;

            rect_ui16_t icon_rc = rc;
            icon_rc.h = icon_rc.w = 30;
            icon_rc.x += 110;
            icon_rc.y += 50;
            render_icon_align(icon_rc, IDR_PNG_wizard_icon_hourglass, COLOR_BLACK, ALIGN_CENTER);

            const uint16_t x = icon_rc.x;
            const uint16_t y = icon_rc.y;

            uint8_t x_start[] = { 15, 13, 11, 15, 15, 10, 11, 12, 15, 6, 12, 11, 12, 13, 13, 14, 14, 15, 10, 6, 6 };
            uint8_t x_end[] = { 15, 16, 19, 15, 15, 19, 19, 18, 15, 23, 17, 19, 18, 17, 16, 16, 15, 15, 19, 23, 23 };
            uint8_t y_start[] = { 24, 33, 13, 19, 29, 33, 13, 14, 24, 33, 32, 13, 14, 15, 16, 17, 18, 26, 31, 32, 33 };
            uint8_t y_end[] = { 28, 33, 13, 23, 33, 33, 13, 14, 28, 33, 32, 13, 14, 15, 16, 17, 18, 33, 31, 32, 33 };
            uint8_t color[] = { 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
            uint32_t colors[] = { COLOR_BLACK, COLOR_ORANGE };
            uint8_t i = 0, limit = 0;

            switch (window->animation) {
            case ANIM_SAND_1:
                limit = 2;
                break;
            case ANIM_SAND_2:
                i = 2;
                limit = 6;
                break;
            case ANIM_SAND_3:
                i = 6;
                limit = 11;
                break;
            case ANIM_SAND_4:
                i = 11;
                limit = 21;
                break;
            default:
                break;
            }

            for (int idx = i; idx < limit; idx++) {
                display::DrawLine(point_ui16(x + x_start[idx], y + y_start[idx]), point_ui16(x + x_end[idx], y + y_end[idx]), colors[color[idx]]);
            }
        }
    }

    if (window->components & DLG_W8_DRAW_PROGRESS) { // progress bar enabled
        if (window->progress_chng) {                 // progress changed
            window->progress_chng = false;

            char text[8];
            rect_ui16_t rc_pro;
            rc_pro.x = 10;
            rc_pro.w = rc.w - 20;
            rc_pro.h = 16;
            rc_pro.y = rc.y + 120;
            display::FillRect(rc_pro, COLOR_GRAY);

            if (window->progress != -1) {
                const uint16_t w = rc_pro.w;
                rc_pro.w = (w * window->progress) / 100;
                display::FillRect(rc_pro, COLOR_ORANGE);
                rc_pro.x += rc_pro.w;
                rc_pro.w = w - rc_pro.w;
                snprintf(text, 8, "%d%%", window->progress);
            } else {
                strlcpy(text, "N/A", 8);
            }

            rc_pro.y += rc_pro.h;
            rc_pro.w = rc.w - 120;
            rc_pro.x = rc.x + 60;
            rc_pro.h = 30;
            // this MakeRAM is safe - text is not necessary after render_text_align finishes its work
            render_text_align(rc_pro, string_view_utf8::MakeRAM((const uint8_t *)text), window->font_title, window->color_back, window->color_text, window->padding, ALIGN_CENTER);
        }
    }
}

void window_dlg_wait_event(window_dlg_wait_t *window, uint8_t event, void *param) {
}

void animation_handler(window_dlg_wait_t *window) {

    if ((HAL_GetTick() - window->timer) >= ANIMATION_MILISEC_DELAY) {

        window->animation++; // change animation frame
        if (window->animation == ANIM_COUNT) {
            window->animation = ANIM_START; // restart animation from begining
        }
        window->animation_chng = true; // redraw sand animation
        window->timer = HAL_GetTick();
        gui_invalidate();
    }
}

void gui_dlg_wait(int8_t (*progress_callback)(), uint8_t comp_flag) {

    window_dlg_wait_t dlg(nullptr, GuiDefaults::RectScreenBodyGet());

    window_t *id_capture = window_t::GetCapturedWindow();
    gui_reset_jogwheel();
    gui_invalidate();
    dlg.SetCapture();

    dlg.progress = (*progress_callback)();
    dlg.components = comp_flag; // holds what component should be drawn
    dlg.timer = HAL_GetTick();

    if (dlg.progress < 0) {
        while ((*progress_callback)()) { // callback that starts with -1 ends with 0
            animation_handler(&dlg);
            gui_loop();
        }
    } else {
        while (dlg.progress < 100) {
            int8_t tmp_progress = (*progress_callback)();
            if (dlg.progress != tmp_progress) {
                dlg.progress = tmp_progress;
                dlg.progress_chng = true;
            }
            animation_handler(&dlg);
            gui_loop();
        }
    }

    window_t *pWin = Screens::Access()->Get();
    if (pWin != 0)
        pWin->Invalidate();
    if (id_capture)
        id_capture->SetCapture();
}
