#pragma once
#include <stdint.h>
#include <cmath>
#include <tuple>
#include "bsod.h"

namespace leds {

struct ColorHSV {
    constexpr ColorHSV(float H, float S, float V)
        : H(H)
        , S(S)
        , V(V) {
    }

    float H {};
    float S {};
    float V {};
};

/// Represents color of an LED
struct Color {
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

    class HSVConverter {
        float H, S, V;
        uint8_t convert(uint8_t n) const {
            float k = fmod(n + H / 60, 6);
            float intermediate = (V * S * std::max(0.f, std::min(std::min(k, 4 - k), 1.f)));
            float e = (V - intermediate);
            return 255 * e;
        }

    public:
        HSVConverter(float H, float S, float V)
            : H(H)
            , S(S / 100)
            , V(V / 100) {}
        std::tuple<uint8_t, uint8_t, uint8_t> Convert() const {
            return { convert(5), convert(3), convert(1) };
        }
    };

    Color(uint32_t data_ = 0)
        : data(data_) {}

    Color(uint8_t r_, uint8_t g_, uint8_t b_)
        : b(b_)
        , r(r_)
        , g(g_)
        , w(0) {}

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
        : b(b)
        , r(r)
        , g(g)
        , w(w) {}

    Color(ColorHSV hsv) {
        if (hsv.H > 360 || hsv.H < 0 || hsv.S > 100 || hsv.S < 0 || hsv.V > 100 || hsv.V < 0) {
            hsv.H = hsv.S = hsv.V = 0;
        }
        HSVConverter converter(hsv.H, hsv.S, hsv.V);
        auto [r_, g_, b_] = converter.Convert();
        r = r_;
        g = g_;
        b = b_;
        w = 0; // TODO
    }

    bool operator==(const Color &other) const {
        Color cmp = other;
        return cmp.data == data;
    }

    bool operator!=(const Color &other) const {
        return !(*this == other);
    }

    Color operator*(double e) const {
        return Color { static_cast<uint8_t>(r * e), static_cast<uint8_t>(g * e), static_cast<uint8_t>(b * e) };
    }
};

} // namespace leds
