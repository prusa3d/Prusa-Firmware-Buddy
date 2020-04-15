/*
 * window_dlg_wait.c
 *
 *  Created on: Nov 5, 2019
 *      Author: Migi
 */
#include "window_dlg_wait.h"
#include "display_helper.h"
#include "gui.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "marlin_client.h"
#include "resource.h"

//dialog flags bitmasks

#define DLG_W8_HOUR_CHNG  0x0300 //Hourglass change flag
#define DLG_W8_HOUR_REDRW 0x0100 //Hourglass sand animation change
#define DLG_W8_ANI_FLG    0x0010 //Hourglass sand animation start
#define DLG_W8_FRAME_FLG  0x4000 //Draw grey frame
#define DLG_W8_PROGRESS   0x8000 //Draw progressbar
#define DLG_W8_HOUR_ROT   0x0200 //rotate hourglass
#define DLG_W8_FPS        2      //frame per sec of hourglass animation

int16_t WINDOW_CLS_DLG_WAIT = 0;

void window_dlg_wait_init(window_dlg_wait_t *window) {
    //if (rect_empty_ui16(window->win.rect)) //use display rect if current rect is empty
    //window->win.rect = rect_ui16(0, 0, display->w, display->h);
    window->win.flg |= WINDOW_FLG_ENABLED; //enabled by default
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;
    window->progress = 0;
}

void window_dlg_wait_draw(window_dlg_wait_t *window) {
    if (window->win.f_visible) {
        rect_ui16_t rc = window->win.rect;

        if (window->win.f_invalid) {
            display->fill_rect(rc, window->color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.y += 10;
            rc_tit.h = 30; // 30pixels for title
            render_text_align(rc_tit, "Please wait", window->font_title, window->color_back, window->color_text, window->padding, ALIGN_CENTER);
            window->win.f_invalid = 0;
            if (window->flags & DLG_W8_FRAME_FLG) { //draw frame
                display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
                display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
                display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
                display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
            }
        }
        if (window->flags & DLG_W8_HOUR_CHNG) { //hourglass animation (just sand 0x0100, whole icon 0x0200)

            rect_ui16_t icon_rc = rc;
            icon_rc.h = icon_rc.w = 30;
            icon_rc.x += 110;
            icon_rc.y += 50;

            if (window->flags & DLG_W8_HOUR_REDRW) {
                render_icon_align(icon_rc, IDR_PNG_wizard_icon_hourglass, COLOR_BLACK, ALIGN_CENTER);
                if (window->flags & 0x0010) {
                    display->draw_line(point_ui16(icon_rc.x + 15, icon_rc.y + 24), point_ui16(icon_rc.x + 15, icon_rc.y + 29), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 13, icon_rc.y + 33), point_ui16(icon_rc.x + 17, icon_rc.y + 33), COLOR_ORANGE);
                } else if (window->flags & 0x0020) {
                    display->draw_line(point_ui16(icon_rc.x + 11, icon_rc.y + 13), point_ui16(icon_rc.x + 20, icon_rc.y + 13), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 15, icon_rc.y + 19), point_ui16(icon_rc.x + 15, icon_rc.y + 24), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 15, icon_rc.y + 29), point_ui16(icon_rc.x + 15, icon_rc.y + 34), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 10, icon_rc.y + 33), point_ui16(icon_rc.x + 20, icon_rc.y + 33), COLOR_ORANGE);
                } else if (window->flags & 0x0040) {
                    display->draw_line(point_ui16(icon_rc.x + 11, icon_rc.y + 13), point_ui16(icon_rc.x + 20, icon_rc.y + 13), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 12, icon_rc.y + 14), point_ui16(icon_rc.x + 19, icon_rc.y + 14), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 15, icon_rc.y + 24), point_ui16(icon_rc.x + 15, icon_rc.y + 29), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 6, icon_rc.y + 33), point_ui16(icon_rc.x + 24, icon_rc.y + 33), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 12, icon_rc.y + 32), point_ui16(icon_rc.x + 18, icon_rc.y + 32), COLOR_ORANGE);
                } else if (window->flags & 0x0080) {
                    display->draw_line(point_ui16(icon_rc.x + 11, icon_rc.y + 13), point_ui16(icon_rc.x + 20, icon_rc.y + 13), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 12, icon_rc.y + 14), point_ui16(icon_rc.x + 19, icon_rc.y + 14), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 13, icon_rc.y + 15), point_ui16(icon_rc.x + 18, icon_rc.y + 15), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 13, icon_rc.y + 16), point_ui16(icon_rc.x + 17, icon_rc.y + 16), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 14, icon_rc.y + 17), point_ui16(icon_rc.x + 17, icon_rc.y + 17), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 14, icon_rc.y + 18), point_ui16(icon_rc.x + 16, icon_rc.y + 18), COLOR_BLACK);
                    display->draw_line(point_ui16(icon_rc.x + 15, icon_rc.y + 26), point_ui16(icon_rc.x + 15, icon_rc.y + 34), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 10, icon_rc.y + 31), point_ui16(icon_rc.x + 20, icon_rc.y + 31), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 6, icon_rc.y + 32), point_ui16(icon_rc.x + 24, icon_rc.y + 32), COLOR_ORANGE);
                    display->draw_line(point_ui16(icon_rc.x + 6, icon_rc.y + 33), point_ui16(icon_rc.x + 24, icon_rc.y + 33), COLOR_ORANGE);
                }
            } else
                render_icon_align(icon_rc, IDR_PNG_wizard_icon_hourglass, COLOR_BLACK, ALIGN_CENTER); //change icon

            window->flags &= 0xF00F; //reset animation flags
        }
        if (window->flags & DLG_W8_PROGRESS) //progress changed
        {
            rect_ui16_t rc_pro = rc;
            rc_pro.x = 10;
            rc_pro.w -= 20;
            char text[16];
            rc_pro.h = 16;
            rc_pro.y += 120;
            uint16_t w = rc_pro.w;
            rc_pro.w = w * window->progress / 100;
            display->fill_rect(rc_pro, COLOR_ORANGE);
            rc_pro.x += rc_pro.w;
            rc_pro.w = w - rc_pro.w;
            display->fill_rect(rc_pro, COLOR_GRAY);
            rc_pro.y += rc_pro.h;
            rc_pro.w = rc.w - 120;
            rc_pro.x = rc.x + 60;
            rc_pro.h = 30;
            sprintf(text, "%d%%", window->progress);
            render_text_align(rc_pro, text, window->font_title, window->color_back, window->color_text, window->padding, ALIGN_CENTER);
            window->flags &= 0x7FFF;
        }
    }
}

void window_dlg_wait_event(window_dlg_wait_t *window, uint8_t event, void *param) {
    //must handle it here
};

const window_class_dlg_wait_t window_class_dlg_wait = {
    {
        WINDOW_CLS_USER,
        sizeof(window_dlg_wait_t),
        (window_init_t *)window_dlg_wait_init,
        0,
        (window_draw_t *)window_dlg_wait_draw,
        (window_event_t *)window_dlg_wait_event,
    },
};

int gui_dlg_wait(int8_t (*callback)()) { //callback

    window_dlg_wait_t dlg;

    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_DLG_WAIT, 0, gui_defaults.msg_box_sz, &dlg);
    window_popup_ptr = (window_t *)&dlg;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);

    dlg.progress = (*callback)();
    dlg.flags |= DLG_W8_FRAME_FLG;  //draw gray frame
    dlg.flags |= DLG_W8_HOUR_REDRW; //redraw hourglass icon

    dlg.timer = HAL_GetTick();
    uint32_t tmp = DLG_W8_ANI_FLG;
    uint8_t tmp_timer, last_time = 0;

    if (dlg.progress < 0) {
        while ((*callback)()) {
            tmp_timer = ((HAL_GetTick() - dlg.timer) / (1000 / DLG_W8_FPS));
            if (tmp_timer != last_time && tmp_timer < 5) {
                if (tmp <= 0x0080) {
                    dlg.flags |= tmp; //redraw sand animation
                    dlg.flags |= DLG_W8_HOUR_REDRW;
                    gui_invalidate();
                }
                if (tmp < 0x0080)
                    tmp <<= 1;
            }
            if (tmp_timer >= 5) {
                dlg.flags |= DLG_W8_HOUR_ROT; //redraw next phase of hourglass animation ADD ICON 0x0200
                dlg.timer = HAL_GetTick();
                tmp = DLG_W8_ANI_FLG;
                tmp_timer = last_time = 0;
                gui_invalidate();
            }
            last_time = tmp_timer;
            gui_loop();
        }
    } else {

        while (dlg.progress < 100) {
            int8_t tmp_progress = (*callback)();
            if (dlg.progress != tmp_progress) {
                dlg.progress = tmp_progress;
                dlg.flags |= DLG_W8_PROGRESS;
            }
            tmp_timer = ((HAL_GetTick() - dlg.timer) / (1000 / DLG_W8_FPS));
            if (tmp_timer != last_time && tmp_timer < 5) {
                if (tmp <= 0x0080) {
                    dlg.flags |= tmp; //redraw sand animation and progressbar
                    dlg.flags |= DLG_W8_HOUR_REDRW;
                    gui_invalidate();
                }
                if (tmp < 0x0080)
                    tmp <<= 1;
            }
            if (tmp_timer >= 5) {
                dlg.flags |= DLG_W8_HOUR_ROT; //redraw next phase of hourglass animation ADD ICON 0x0200
                dlg.timer = HAL_GetTick();
                tmp = DLG_W8_ANI_FLG;
                tmp_timer = last_time = 0;
                gui_invalidate();
            }
            last_time = tmp_timer;
            gui_loop();
        }
    }

    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
    return 0;
}

int8_t gui_marlin_busy_callback() {
    if (marlin_motion() || marlin_busy())
        return -1;
    else
        return 0;
}
