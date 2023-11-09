// window_term.cpp
#include "window_term.hpp"
#include "gui.hpp"
#include <stdarg.h> //va_list

window_term_t::window_term_t(window_t *parent, point_i16_t pt, uint8_t *buff, size_t cols, size_t rows)
    : window_term_t(parent, pt, buff, cols, rows, GuiDefaults::Font) {
}

window_term_t::window_term_t(window_t *parent, point_i16_t pt, uint8_t *buff, size_t cols, size_t rows, font_t *fnt)
    : AddSuperWindow<window_t>(parent, Rect16(pt, fnt->w * cols, fnt->h * rows))
    , color_text(GuiDefaults::ColorText)
    , font(fnt) {
    term_init(&term, cols, rows, buff);
}

void window_term_t::unconditionalDraw() {
    if (term.flg & TERM_FLG_CHANGED) {
        render_term(&term, Left(), Top(), font, GetBackColor(), color_text);
    } else {
        super::unconditionalDraw();
    }
}

int window_term_t::Printf(const char *fmt, ...) {
    char text[TERM_PRINTF_MAX];

    va_list va;
    va_start(va, fmt);
    int ret = vsnprintf(text, sizeof(text), fmt, va);
    va_end(va);

    const size_t range = std::min(ret, TERM_PRINTF_MAX);
    for (size_t i = 0; i < range; i++) {
        term_write_char(&term, text[i]);
    }

    Invalidate();
    return ret;
}

void window_term_t::WriteChar(uint8_t ch) {
    term_write_char(&term, ch);
    Invalidate();
}

void render_term(term_t *pterm, size_t x, size_t y, const font_t *font, color_t color_back, color_t color_text) {
    uint8_t char_w = font->w;
    uint8_t char_h = font->h;
    if (pterm->flg & TERM_FLG_CHANGED) {
        uint8_t *pb = pterm->buff;
        uint8_t *pm = pterm->buff + (pterm->cols * pterm->rows * 2);
        uint8_t msk = 0x01;
        uint8_t c;
        int i = 0;
        for (uint8_t r = 0; r < pterm->rows; ++r) {
            for (c = 0; c < pterm->cols; ++c) {
                if ((*pm) & msk) {
                    // character is followed by attribute
                    uint8_t ch = *(pb++);
                    pb++; // uint8_t attr = *(pb++);
                    display::DrawChar(point_ui16(x + c * char_w, y + r * char_h), ch, font, color_back, color_text);
                } else {
                    pb += 2;
                }
                i++;
                msk <<= 1;
                if ((i & 7) == 0) {
                    *pm = 0;
                    pm++;
                    msk = 0x01;
                }
            }
        }
    }
}
