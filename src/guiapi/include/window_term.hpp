// window_term.hpp
#pragma once

#include "window.hpp"
#include "term.h"

struct window_class_term_t {
    window_class_t cls;
};

struct window_term_t : public window_t {
    color_t color_text;
    font_t *font;
    term_t *term;
    window_term_t(window_t *parent, window_t *prev);
};

extern const window_class_term_t window_class_term;
void render_term(rect_ui16_t rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1);
