// window_qr.hpp

#pragma once

#include "window.hpp"

#include "qrcodegen.h"

struct window_qr_t : public window_t {
    char *text;
    int version;
    enum qrcodegen_Ecc ecc_level;
    enum qrcodegen_Mode mode;
    uint8_t border;
    uint8_t px_per_module;
    color_t bg_color;
    color_t px_color;

    window_qr_t(window_t *parent, window_t *prev, rect_ui16_t rect);
};
