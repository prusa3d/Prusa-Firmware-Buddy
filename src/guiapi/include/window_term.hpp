// window_term.hpp
#pragma once

#include "window.hpp"
#include "term.h"

typedef struct _window_class_term_t {
    window_class_t cls;
} window_class_term_t;

struct window_term_t : public window_t {
    color_t color_back;
    color_t color_text;
    font_t *font;
    term_t *term;
};

extern const window_class_term_t window_class_term;
void render_term(rect_ui16_t rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1);
