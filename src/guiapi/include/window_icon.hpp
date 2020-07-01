// window_icon.hpp

#pragma once

#include "window.hpp"

struct window_icon_t : public window_t {
    uint16_t id_res;
    uint8_t alignment;
    uint16_t GetIdRes() const { return id_res; }
    void SetIdRes(int16_t id);
};

struct window_class_icon_t {
    window_class_t cls;
};

extern const window_class_icon_t window_class_icon;
