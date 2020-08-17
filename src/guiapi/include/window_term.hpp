// window_term.hpp
#pragma once

#include "window.hpp"
#include "term.h"

//todo, why is this using 2nd class term_t?
//to be sizeable?
class window_term_t : public window_t {
public:
    color_t color_text;
    font_t *font;
    term_t *term;

    window_term_t(window_t *parent, Rect16 rect);

protected:
    virtual void unconditionalDraw() override;
};

void render_term(Rect16 rc, term_t *pt, const font_t *font, color_t clr0, color_t clr1);
