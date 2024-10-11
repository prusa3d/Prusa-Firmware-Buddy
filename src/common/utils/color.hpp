#pragma once

#include <cstdint>
#include <optional>
#include <algorithm>
#include <string_view>
#include <bit>

struct Color {

public:
    constexpr Color() = default;
    constexpr Color(const Color &) = default;

    static constexpr Color from_rgb(uint8_t r, uint8_t g, uint8_t b) {
        return from_raw(b | (g << 8) | (r << 16));
    }

    static constexpr Color from_raw(uint32_t raw) {
        Color c;
        c.raw = raw;
        return c;
    }

    static Color mix(Color back, Color front, uint8_t front_alpha);

    /// Looks up the \p str in the color table and returns an appropriate color.
    /// If the string starts with '#', interprets it as a hex value
    /// If the \p str is not in the color table, interprets it as a decimal string and tries to construct a color from that
    static std::optional<Color> from_string(const std::string_view &str);

    static std::optional<Color> from_gcode_param(const std::string_view &value) {
        return from_string(value);
    }

    /// Converts RGB colour to grayscale luminosity. 255 == all white, 0 == all black
    uint8_t to_grayscale() const {
        return std::min<uint16_t>((77 * static_cast<uint16_t>(r) + 150 * static_cast<uint16_t>(g) + 29 * static_cast<uint16_t>(b)) >> 8, 255);
    }

public:
    inline bool operator==(const Color &o) const {
        return raw == o.raw;
    }
    inline bool operator!=(const Color &o) const {
        return raw != o.raw;
    }

public:
    union {
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t _unused;
        };
        uint32_t raw = 0;
    };
    // The order of BGR depends on correct endianess
    static_assert(std::endian::native == std::endian::little);
};

constexpr Color COLOR_BLACK = Color::from_raw(0x000000);
constexpr Color COLOR_WHITE = Color::from_raw(0xffffff);
constexpr Color COLOR_RED = Color::from_raw(0xff0000);
constexpr Color COLOR_RED_ALERT = Color::from_raw(0xe74626);
constexpr Color COLOR_LIME = Color::from_raw(0x00ff00);
constexpr Color COLOR_BLUE = Color::from_raw(0x0000ff);
constexpr Color COLOR_AZURE = Color::from_raw(0x129dff);
constexpr Color COLOR_YELLOW = Color::from_raw(0xffff00);
constexpr Color COLOR_CYAN = Color::from_raw(0x00ffff);
constexpr Color COLOR_MAGENTA = Color::from_raw(0xff00ff);
constexpr Color COLOR_SILVER = Color::from_raw(0xc0c0c0);
constexpr Color COLOR_LIGHT_GRAY = Color::from_raw(0xAAAAAA);
constexpr Color COLOR_VERY_LIGHT_GRAY = Color::from_raw(0xCCCCCC);
constexpr Color COLOR_GRAY = Color::from_raw(0x808080);
constexpr Color COLOR_DARK_GRAY = Color::from_raw(0x5B5B5B);
constexpr Color COLOR_VERY_DARK_GRAY = Color::from_raw(0x222222);
constexpr Color COLOR_MAROON = Color::from_raw(0x800000);
constexpr Color COLOR_OLIVE = Color::from_raw(0x808000);
constexpr Color COLOR_GREEN = Color::from_raw(0x008000);
constexpr Color COLOR_DARK_GREEN = Color::from_raw(0x006000);
constexpr Color COLOR_LIGHT_GREEN = Color::from_raw(0x40b040);
constexpr Color COLOR_PURPLE = Color::from_raw(0x800080);
constexpr Color COLOR_TEAL = Color::from_raw(0x008080);
constexpr Color COLOR_NAVY = Color::from_raw(0x000080);
constexpr Color COLOR_ORANGE = Color::from_raw(0xF8651B);
constexpr Color COLOR_DARK_KHAKI = Color::from_raw(0xDBD76B);
