// window_term.cpp
#include "window_term.hpp"
#include "gui.hpp"
#include <stdarg.h> //va_list

window_term_t::window_term_t(window_t *parent, point_i16_t pt, uint8_t *buff, size_t cols, size_t rows)
    : window_term_t(parent, pt, buff, cols, rows, GuiDefaults::Font) {
}

window_term_t::window_term_t(window_t *parent, point_i16_t pt, uint8_t *buff, size_t cols, size_t rows, font_t *fnt)
    : window_t(parent, Rect16(pt, font->w * cols, font->h * rows))
    , color_text(GuiDefaults::ColorText)
    , font(fnt) {
    term_init(&term, cols, rows, buff);
}

void window_term_t::unconditionalDraw() {
    uint8_t char_w = font->w;
    uint8_t char_h = font->h;
    if (term.flg & TERM_FLG_CHANGED) {
        uint8_t *pb = term.buff;
        uint8_t *pm = term.buff + (term.cols * term.rows * 2);
        uint8_t msk = 0x01;
        uint8_t c;
        int i = 0;
        for (uint8_t r = 0; r < term.rows; ++r)
            for (c = 0; c < term.cols; ++c) {
                if ((*pm) & msk) {
                    //character is followed by attribute
                    uint8_t ch = *(pb++);
                    pb++; //uint8_t attr = *(pb++);
                    display::DrawChar(point_ui16(rect.Left() + c * char_w, rect.Top() + r * char_h), ch, font, color_back, color_text);
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
        display::FillRect(rect, color_back);
}

int window_term_t::Printf(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    char text[TERM_PRINTF_MAX];

    int ret = vsnprintf(text, sizeof(text), fmt, va);

    const size_t range = std::min(ret, TERM_PRINTF_MAX);
    for (size_t i = 0; i < range; i++)
        term_write_char(&term, text[i]);

    va_end(va);
    Invalidate();
    return ret;
}

void window_term_t::WriteChar(uint8_t ch) {
    term_write_char(&term, ch);
    Invalidate();
}
