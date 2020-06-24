// window_text.hpp

#pragma once

#include "window.hpp"

struct window_class_text_t {
    window_class_t cls;
};

struct window_text_t : public window_t {
    color_t color_back;
    color_t color_text;
    font_t *font;
    const char *text;
    padding_ui8_t padding;
    uint8_t alignment;

    const char *GetText() const { return text; }
    void SetText(const char *txt) {
        text = txt;
        _window_invalidate(this);
    }
    void SetTextColor(color_t clr) {
        color_text = clr;
        _window_invalidate(this);
    }
};

extern const window_class_text_t window_class_text;
