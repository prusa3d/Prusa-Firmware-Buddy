// window_term.c
#include "window_term.hpp"
#include "gui.hpp"

void render_term(Rect16 rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1) {
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
                    display::DrawChar(point_ui16(rc.Left() + c * char_w, rc.Top() + r * char_h), ch, font, clr0, clr1);
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

void window_term_t::unconditionalDraw() {
    render_term(rect, term, font, color_back, color_text);
}

window_term_t::window_term_t(window_t *parent, Rect16 rect)
    : window_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , term(0) {
}
