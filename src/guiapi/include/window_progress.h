// window_progress.h

#pragma once

#include "window.hpp"

typedef struct _window_class_progress_t {
    window_class_t cls;
} window_class_progress_t;

typedef struct _window_progress_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    color_t color_progress;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    uint8_t height_progress;
    const char *format;
    float value;
    float min;
    float max;
} window_progress_t;

extern const window_class_progress_t window_class_progress;
