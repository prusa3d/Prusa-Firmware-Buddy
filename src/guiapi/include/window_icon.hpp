// window_icon.hpp

#pragma once

#include "window.hpp"

typedef struct _window_icon_t : public window_t {
    color_t color_back;
    uint16_t id_res;
    uint8_t alignment;
} window_icon_t;

typedef struct _window_class_icon_t {
    window_class_t cls;
} window_class_icon_t;

extern const window_class_icon_t window_class_icon;
