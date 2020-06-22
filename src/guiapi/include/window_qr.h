// window_qr.h

#pragma once

#include "window.hpp"

#include "qrcodegen.h"

typedef struct {
    window_class_t cls;
} window_class_qr_t;

typedef struct {
    window_t win;
    char *text;
    int version;
    enum qrcodegen_Ecc ecc_level;
    enum qrcodegen_Mode mode;
    uint8_t border;
    uint8_t px_per_module;
    color_t bg_color;
    color_t px_color;
} window_qr_t;

extern const window_class_qr_t window_class_qr;
