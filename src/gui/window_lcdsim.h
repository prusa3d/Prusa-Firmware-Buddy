// window_lcdsim.h

#pragma once
#include "window.h"

typedef struct _window_class_lcdsim_t {
    window_class_t cls;
} window_class_lcdsim_t;

typedef struct _window_lcdsim_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
} window_lcdsim_t;

extern int16_t WINDOW_CLS_LCDSIM;

//extern int16_t window_lcdsim_create(int16_t id_parent, rect_ui16_t rc, font_t* font, color_t color_back, color_t color_text);

extern const window_class_lcdsim_t window_class_lcdsim;
