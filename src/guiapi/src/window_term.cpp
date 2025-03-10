#include "window_term.hpp"

#include "display.hpp"
#include "gui.hpp"
#include <stdarg.h> //va_list

static constexpr Font font = GuiDefaults::DefaultFont;
static void render_term(term_t *pterm, size_t x, size_t y, Color color_back, Color color_text);

window_term_t::window_term_t(window_t *parent, point_i16_t pt, uint8_t *buff, size_t cols, size_t rows)
    : window_t(parent, Rect16(pt, width(font) * cols, height(font) * rows))
    , color_text(GuiDefaults::ColorText) {
    term_init(&term, cols, rows, buff);
}

void window_term_t::unconditionalDraw() {
    if (term.flg & TERM_FLG_CHANGED) {
        render_term(&term, Left(), Top(), GetBackColor(), color_text);
    } else {
        window_t::unconditionalDraw();
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

static void render_term(term_t *pterm, size_t x, size_t y, Color color_back, Color color_text) {
    uint8_t char_w = width(font);
    uint8_t char_h = height(font);
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
                    display::draw_char(point_ui16(x + c * char_w, y + r * char_h), ch, resource_font(font), color_back, color_text);
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
