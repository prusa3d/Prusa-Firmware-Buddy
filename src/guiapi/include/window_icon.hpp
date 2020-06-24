// window_icon.hpp

#pragma once

#include "window.hpp"

struct window_icon_t : public window_t {
    uint16_t id_res;
    uint8_t alignment;
};

struct window_class_icon_t {
    window_class_t cls;
};

extern const window_class_icon_t window_class_icon;
