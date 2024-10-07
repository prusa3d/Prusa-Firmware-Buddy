#pragma once
#include <stdint.h>
#include <cmath>
#include <tuple>
#include "bsod.h"

namespace leds {

struct ColorHSV {
    float h = 0;
    float s = 0;
    float v = 0;
};

/// Represents color of an LED
struct ColorRGBW {
    union {
        uint32_t data;
        struct {
            // DO NOT CHANGE ORDER !!!
            // BFW-4994 this should probably be done better
            uint8_t b;
            uint8_t r;
            uint8_t g;
            uint8_t w; // only if supported
        };
    };

    ColorRGBW(uint32_t data_ = 0)
        : data(data_) {}

    ColorRGBW(uint8_t r_, uint8_t g_, uint8_t b_)
        : b(b_)
        , r(r_)
        , g(g_)
        , w(0) {}

    ColorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
        : b(b)
        , r(r)
        , g(g)
        , w(w) {}

    static ColorRGBW from_hsv(ColorHSV hsv);

    bool operator==(const ColorRGBW &other) const {
        ColorRGBW cmp = other;
        return cmp.data == data;
    }

    bool operator!=(const ColorRGBW &other) const {
        return !(*this == other);
    }

    ColorRGBW operator*(double e) const {
        return ColorRGBW { static_cast<uint8_t>(r * e), static_cast<uint8_t>(g * e), static_cast<uint8_t>(b * e) };
    }
};

} // namespace leds
