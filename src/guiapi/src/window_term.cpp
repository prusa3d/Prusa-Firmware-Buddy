// window_term.c
#include "window_term.h"
#include "gui.h"

static void window_term_init(window_term_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->term = 0;
    window->win.flg |= WINDOW_FLG_ENABLED;

    display::FillRect(window->win.rect, window->color_back);
}

void render_term(rect_ui16_t rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1) {
    uint8_t char_w = font->w;
    uint8_t char_h = font->h;
    if (pt && (pt->flg & TERM_FLG_CHANGED)) {
        const uint8_t cols = pt->cols;
        const uint8_t rows = pt->rows;
        uint8_t *pb = pt->buff;
        uint8_t *pm = pt->buff + (cols * rows * 2);
        uint8_t msk = 0x01;
        uint8_t c;
        int i = 0;
        for (uint8_t r = 0; r < rows; ++r)
            for (c = 0; c < cols; ++c) {
                if ((*pm) & msk) {
                    //character is followed by attribute
                    uint8_t ch = *(pb++);
                    pb++; //uint8_t attr = *(pb++);
                    display::DrawChar(point_ui16(rc.x + c * char_w, rc.y + r * char_h), ch, font, clr0, clr1);
                } else
                    pb += 2;
                i++;
                msk <<= 1;
                if ((i & 7) == 0) {
                    *pm = 0;
                    pm++;
                    msk = 0x01;
                }
            }
    } else
        display::FillRect(rc, clr0);
}

static void window_term_draw(window_term_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        render_term(window->win.rect, window->term, window->font, window->color_back, window->color_text);
        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_term_t window_class_term = {
    {
        WINDOW_CLS_TERM,
        sizeof(window_term_t),
        (window_init_t *)window_term_init,
        0,
        (window_draw_t *)window_term_draw,
        0,
    },
};
