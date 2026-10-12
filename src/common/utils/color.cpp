#include "color.hpp"

#include <array>
#include <cmath>

#include <str_utils.hpp>

struct ColorPreset {
    std::string_view name;
    Color color;
};

static constexpr auto color_presets = std::to_array<ColorPreset>({
    { "BLACK", Color::from_raw(0x000000) },
    { "BLUE", Color::from_raw(0x0000FF) },
    { "GREEN", Color::from_raw(0x00FF00) },
    { "BROWN", Color::from_raw(0x804000) },
    { "PURPLE", Color::from_raw(0x800080) },
    { "GRAY", Color::from_raw(0x999999) },
    { "TERRACOTTA", Color::from_raw(0xB87F6A) },
    { "SILVER", Color::from_raw(0xC0C0C0) },
    { "GOLD", Color::from_raw(0xD4AF37) },
    { "RED", Color::from_raw(0xFF0000) },
    { "PINK", Color::from_raw(0xFF007F) },
    { "ORANGE", Color::from_raw(0xFF8000) },
    { "TRANSPARENT", Color::from_raw(0xF0F0F0) },
    { "YELLOW", Color::from_raw(0xFFFF00) },
    { "WHITE", Color::from_raw(0xFFFFFF) },
});

Color Color::mix(Color back, Color front, uint8_t front_alpha) {
    // Technically correct would be "/ 255", but difference to ">> 8" is less than 1.

    const uint8_t front_alpha_inv = 255 - front_alpha;
    Color result;
    result.b = (front_alpha * front.b + front_alpha_inv * back.b) >> 8;
    result.g = (front_alpha * front.g + front_alpha_inv * back.g) >> 8;
    result.r = (front_alpha * front.r + front_alpha_inv * back.r) >> 8;
    return result;
}

float Color::perceptual_distance(Color a, Color b) {
    // Channel weights in 1/256 units, sliding with how red the two colors are.
    // Empirical fit of human perception, see https://www.compuphase.com/cmetric.htm
    const auto compute_dist_sq = [](Color a2, Color b2) -> uint32_t {
        const int32_t r_mean = (int32_t(a2.r) + b2.r) / 2;
        const int32_t dr = int32_t(a2.r) - b2.r;
        const int32_t dg = int32_t(a2.g) - b2.g;
        const int32_t db = int32_t(a2.b) - b2.b;

        return (512 + r_mean) * dr * dr + 1024 * dg * dg + (767 - r_mean) * db * db;
    };

    constexpr float max_dist_sq = float(compute_dist_sq(Color ::from_rgb(0, 0, 0), Color::from_rgb(255, 255, 255)));
    constexpr float max_dist_sq_inv = 1.0f / max_dist_sq;

    return std::sqrt(float(compute_dist_sq(a, b)) * max_dist_sq_inv);
}

std::optional<Color> Color::from_string(const std::string_view &str) {
    if (str.size() == 0) {
        return std::nullopt;
    }

    if (Color c; str.starts_with('#') && from_chars_light(str.begin() + 1, str.end(), c.raw, 16).ec == std::errc {}) {
        c._unused = 0;
        return c;
    }

    // Color preset
    for (const ColorPreset &c : color_presets) {
        if (str == c.name) {
            return c.color;
        }
    }

    // Otherwise treat as number for legacy reasons
    if (Color c; from_chars_light(str.begin(), str.end(), c.raw).ec == std::errc {}) {
        c._unused = 0;
        return c;
    }

    return std::nullopt;
}
