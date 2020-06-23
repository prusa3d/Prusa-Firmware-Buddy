#pragma once

#include "window_menu.h"
#include <stdint.h>

struct IWindowMenu : window_t {
    color_t color_back;
    color_t color_text;
    color_t color_disabled;
    font_t *font;
    padding_ui8_t padding;
    rect_ui16_t icon_rect;
    uint8_t alignment;
};
