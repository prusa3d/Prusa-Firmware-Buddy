// window_text.hpp

#pragma once

#include "window.hpp"

typedef struct _window_class_text_t {
    window_class_t cls;
} window_class_text_t;

typedef struct _window_text_t : public window_t {
    color_t color_back;
    color_t color_text;
    font_t *font;
    char *text;
    padding_ui8_t padding;
    uint8_t alignment;
} window_text_t;

extern const window_class_text_t window_class_text;
