#pragma once

#include "window.hpp"
#include <stdint.h>

struct IWindowMenu : public window_aligned_t {
    color_t color_text;
    color_t color_disabled;
    font_t *font;
    padding_ui8_t padding;
    uint8_t icon_w;
    IWindowMenu(window_t *parent, Rect16 rect);
};
