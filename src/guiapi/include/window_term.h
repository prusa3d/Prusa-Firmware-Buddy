// window_term.h
#ifndef _WINDOW_TERM_H
#define _WINDOW_TERM_H

#include "window.hpp"
#include "term.h"

typedef struct _window_class_term_t {
    window_class_t cls;
} window_class_term_t;

typedef struct _window_term_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    term_t *term;
} window_term_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_term_t window_class_term;
void render_term(rect_ui16_t rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _WINDOW_TERM_H
