#include "led_color.hpp"

namespace leds {

namespace {

    // operator== reads the union's data member, which is not allowed in
    // constant expressions when the channels were the ones initialized
    constexpr bool channels_equal(ColorRGBW a, ColorRGBW b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
    }

    static_assert(channels_equal(ColorRGBW::from_hsv({ 0, 100, 100 }), ColorRGBW(255, 0, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 120, 100, 100 }), ColorRGBW(0, 255, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 240, 100, 100 }), ColorRGBW(0, 0, 255)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 360, 100, 100 }), ColorRGBW(255, 0, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 0, 0, 100 }), ColorRGBW(255, 255, 255)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 0, 100, 0 }), ColorRGBW(0, 0, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 361, 100, 100 }), ColorRGBW(0, 0, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 0, 101, 100 }), ColorRGBW(0, 0, 0)));
    static_assert(channels_equal(ColorRGBW::from_hsv({ 0, 100, -1 }), ColorRGBW(0, 0, 0)));

} // namespace

ColorRGBW ColorRGBW::fade(float brightness) const {
    uint8_t nr = static_cast<uint8_t>(r * brightness);
    uint8_t ng = static_cast<uint8_t>(g * brightness);
    uint8_t nb = static_cast<uint8_t>(b * brightness);
    uint8_t nw = static_cast<uint8_t>(w * brightness);
    return { nr, ng, nb, nw };
}

ColorRGBW ColorRGBW::clamp(uint8_t max_level) const {
    using std::min;
    return { min(r, max_level), min(g, max_level), min(b, max_level), min(w, max_level) };
}

ColorRGBW ColorRGBW::cross_fade(const ColorRGBW &c, float ratio) const {
    float old_ratio = std::max(0.5f - ratio, 0.0f) * 2;
    float new_ratio = std::max(ratio - 0.5f, 0.0f) * 2;
    uint8_t nr = static_cast<uint8_t>(r * old_ratio + c.r * new_ratio);
    uint8_t ng = static_cast<uint8_t>(g * old_ratio + c.g * new_ratio);
    uint8_t nb = static_cast<uint8_t>(b * old_ratio + c.b * new_ratio);
    uint8_t nw = static_cast<uint8_t>(w * old_ratio + c.w * new_ratio);
    return { nr, ng, nb, nw };
}

ColorRGBW ColorRGBW::blend(const ColorRGBW &c, float ratio) const {
    float rratio = 1.0f - ratio;
    uint8_t nr = static_cast<uint8_t>(r * rratio + c.r * ratio);
    uint8_t ng = static_cast<uint8_t>(g * rratio + c.g * ratio);
    uint8_t nb = static_cast<uint8_t>(b * rratio + c.b * ratio);
    uint8_t nw = static_cast<uint8_t>(w * rratio + c.w * ratio);
    return { nr, ng, nb, nw };
}

} // namespace leds
