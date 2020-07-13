// window_icon.hpp

#pragma once

#include "window.hpp"

struct window_icon_t : public window_t {
    uint16_t id_res;
    uint8_t alignment;
    uint16_t GetIdRes() const { return id_res; }
    void SetIdRes(int16_t id);

    window_icon_t(window_t *parent, window_t *prev, rect_ui16_t rect = { 0 }, uint16_t id_res = 0);
};

struct window_class_icon_t {
    window_class_t cls;
};

extern const window_class_icon_t window_class_icon;
