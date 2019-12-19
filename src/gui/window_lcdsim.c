// window_lcdsim.c
#include "window_lcdsim.h"
#include "lcdsim.h"
#include "gui.h"

int16_t WINDOW_CLS_LCDSIM = 0;

void window_lcdsim_init(window_lcdsim_t *window) {
    window->color_back = COLOR_BLUE;
    window->color_text = COLOR_WHITE;
    //window->font = GUI_DEF_FONT;
    window->font = resource_font(IDR_FNT_BIG);
}

void window_lcdsim_done(window_lcdsim_t *window) {
}

void window_lcdsim_draw(window_lcdsim_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        font_t font2 = { 8, 8, 1, 0 /*FONT_FLG_LSBF*/, lcdsim_user_charset_ptr(), 0, 7 };
        uint16_t x = window->win.rect.x;
        uint16_t y = window->win.rect.y;
        font_t *font = window->font;
        uint8_t ch_w = font->w;
        uint8_t ch_h = font->h;
        color_t clr0 = window->color_back;
        color_t clr1 = window->color_text;
        uint32_t inval;
        uint32_t mask;
        uint8_t ch;
        uint8_t c;
        uint8_t r;
        for (r = 0; r < 4; r++) {
            inval = lcdsim_inval_mask[r];
            lcdsim_inval_mask[r] &= ~inval;
            if (inval) {
                mask = 0x01;
                for (c = 0; c < 20; c++) {
                    if (inval & mask) {
                        ch = lcdsim_char_at(c, r);
                        if ((ch >= 32) && (ch <= 127))
                            display->draw_char(point_ui16(x + c * ch_w, y + r * ch_h), ch, font, clr0, clr1);
                        else if (ch < 8) {
                            display->fill_rect(rect_ui16(x + c * ch_w, y + r * ch_h, ch_w, ch_h), clr0);
                            display->draw_char(point_ui16(x + c * ch_w, y + r * ch_h), ch, &font2, clr0, clr1);
                            //display->draw_char(point_ui16(x + c * ch_w, y + r * ch_h), '0'+ch, font, clr0, COLOR_RED);
                        } else
                            display->fill_rect(rect_ui16(x + c * ch_w, y + r * ch_h, ch_w, ch_h), COLOR_RED);
                    }
                    mask <<= 1;
                }
            }
        }
        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_lcdsim_t window_class_lcdsim = {
    {
        WINDOW_CLS_USER,
        sizeof(window_lcdsim_t),
        (window_init_t *)window_lcdsim_init,
        (window_init_t *)window_lcdsim_done,
        (window_init_t *)window_lcdsim_draw,
        0,
    },
};
