/*
 * window_logo.c
 *
 *  Created on: 28. 8. 2019
 *      Author: mcbig
 */

#include "window_logo.h"
#include "config.h"

int16_t WINDOW_CLS_LOGO = 0;

void window_logo_init(window_logo_t *window) {
    window->color_back = gui_defaults.color_back;
}

void window_logo_done(window_logo_t *window) {}

void window_logo_draw(window_logo_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)))

    {
        font_t *logo = resource_font(IDR_FNT_original_prusa);
#if PRINTER_TYPE == PRINTER_PRUSA_MINI
        const char chr = '\2';
        const uint16_t left = 16;
        const uint16_t right = 105;
#endif

        // ORIGINAL
        display->draw_char(point_ui16(left, window->win.rect.y),
            '\0', logo, window->color_back, COLOR_GRAY);
        // TYPE
        display->draw_char(point_ui16(right, window->win.rect.y + 19),
            chr, logo, window->color_back, COLOR_ORANGE);

        // PRUSA
        display->draw_char(point_ui16(left, window->win.rect.y + 19),
            '\1', logo, window->color_back, COLOR_WHITE);

#ifdef _DEBUG
        // DEBUG
        display->draw_text(rect_ui16(160, window->win.rect.y, 80, 22),
            "DEBUG", resource_font(IDR_FNT_NORMAL), window->color_back, COLOR_RED);
#endif //_DEBUG

        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_logo_t window_class_logo = {
    {
        WINDOW_CLS_USER,
        sizeof(window_logo_t),
        (window_init_t *)window_logo_init,
        (window_done_t *)window_logo_done,
        (window_draw_t *)window_logo_draw,
        0,
    },
};
