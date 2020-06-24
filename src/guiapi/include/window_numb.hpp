// window_numb.hpp

#pragma once

#include "window.hpp"

#define WINDOW_FLG_NUMB_FLOAT2INT (WINDOW_FLG_USER << 1)

struct window_class_numb_t {
    window_class_t cls;
};

struct window_numb_t : public window_t {
    color_t color_text;
    font_t *font;
    float value;
    const char *format;
    padding_ui8_t padding;
    uint8_t alignment;

    void SetFormat(const char *frmt);
    const char *GetFormat() { return format; }
};

extern const window_class_numb_t window_class_numb;
