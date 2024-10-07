#include <leds/color.hpp>

namespace leds {

ColorRGBW ColorRGBW::from_hsv(ColorHSV hsv) {
    if (hsv.h > 360 || hsv.h < 0 || hsv.s > 100 || hsv.s < 0 || hsv.v > 100 || hsv.v < 0) {
        return ColorRGBW();
    }

    const float h = hsv.h;
    const float s = hsv.s / 100.0f;
    const float v = hsv.v / 100.0f;

    const auto convert = [&](uint8_t n) -> uint8_t {
        float k = fmod(n + h / 60, 6);
        float intermediate = (v * s * std::max(0.f, std::min(std::min(k, 4 - k), 1.f)));
        float e = (v - intermediate);
        return 255 * e;
    };

    return ColorRGBW(convert(5), convert(3), convert(1), 0);
}

} // namespace leds
