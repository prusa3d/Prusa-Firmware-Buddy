// window_qr.hpp

#pragma once

#include "window.hpp"
//#include "qrcodegen.h"

struct window_class_qr_t {
    window_class_t cls;
};

struct window_qr_t : public window_t {
    // 32 bit
    color_t bg_color;
    color_t px_color;
    // 8 bit
    char *text;
    uint8_t border;
    uint8_t px_per_module;
    uint8_t align;
};

extern const window_class_qr_t window_class_qr;

/// Defines maximal size of QR code and buffers needed for generating. Keep it low.
constexpr uint8_t qr_version_max = 9;
