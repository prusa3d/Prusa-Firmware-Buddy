#include "color.hpp"

#include <array>
#include <string>
#include <cinttypes>

struct ColorPreset {
    std::string_view name;
    Color color;
};

static constexpr auto color_presets = std::to_array<ColorPreset>({
    { "BLACK", Color::from_raw(0x000000) },
    { "BLUE", Color::from_raw(0x0000FF) },
    { "GREEN", Color::from_raw(0x00FF00) },
    { "BROWN", Color::from_raw(0x800000) },
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

std::optional<Color> Color::from_string(const std::string_view &str) {
    if (str.size() == 0) {
        return std::nullopt;
    }

    if (Color c; str.starts_with('#') && sscanf(str.data(), "#%06" PRIX32, &c.raw) == 1) {
        return c;
    }

    // Color preset
    for (const ColorPreset &c : color_presets) {
        if (str == c.name) {
            return c.color;
        }
    }

    // Otherwise treat as number for legacy reasons
    if (Color c; sscanf(str.data(), "%" PRIu32, &c.raw) == 1) {
        return c;
    }

    return std::nullopt;
}
