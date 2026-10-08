#pragma once

#include <algorithm>
#include <cstdint>

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

    constexpr ColorRGBW(uint32_t data_ = 0)
        : data(data_) {}

    constexpr ColorRGBW(uint8_t r_, uint8_t g_, uint8_t b_)
        : b(b_)
        , r(r_)
        , g(g_)
        , w(0) {}

    constexpr ColorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
        : b(b)
        , r(r)
        , g(g)
        , w(w) {}

    static constexpr ColorRGBW from_hsv(ColorHSV hsv) {
        if (hsv.h > 360 || hsv.h < 0 || hsv.s > 100 || hsv.s < 0 || hsv.v > 100 || hsv.v < 0) {
            return ColorRGBW(0, 0, 0, 0);
        }

        const float h = hsv.h;
        const float s = hsv.s / 100.0f;
        const float v = hsv.v / 100.0f;

        const auto convert = [&](uint8_t n) -> uint8_t {
            // n + h / 60 is within [0, 11], so a single wrap is enough
            float k = n + h / 60;
            if (k >= 6) {
                k -= 6;
            }
            float intermediate = (v * s * std::max(0.f, std::min(std::min(k, 4 - k), 1.f)));
            float e = (v - intermediate);
            return static_cast<uint8_t>(255 * e);
        };

        return ColorRGBW(convert(5), convert(3), convert(1), 0);
    }

    bool operator==(const ColorRGBW &other) const {
        return data == other.data;
    }

    bool operator!=(const ColorRGBW &other) const {
        return !(*this == other);
    }

    ColorRGBW operator*(double e) const {
        return ColorRGBW { static_cast<uint8_t>(r * e), static_cast<uint8_t>(g * e), static_cast<uint8_t>(b * e) };
    }

    ColorRGBW fade(float brightness) const;

    ColorRGBW clamp(uint8_t max_level) const;

    ColorRGBW cross_fade(const ColorRGBW &c2, float ratio) const;

    ColorRGBW blend(const ColorRGBW &c2, float ratio) const;
};

} // namespace leds
