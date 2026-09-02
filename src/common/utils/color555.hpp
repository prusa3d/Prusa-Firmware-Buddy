/// @file
#pragma once

#include <cstdint>

#include <utils/color.hpp>
#include <utils/compact_optional.hpp>

/// More compact encoding of a color - used in config store
/// !!! DO NOT CHANGE LAYOUT, USED IN CONFIG STORE
struct Color555 {
    uint16_t raw = 0;

    /// Used for compact_optional, do not use on your own
    static constexpr uint16_t invalid_bit = (1 << 15);

    static constexpr Color555 from_color(Color c) {
        static_assert(
            requires { c._unused; }, "Handle newly added alpha channel");
        return from_rgb(c.r, c.g, c.b);
    }

    static constexpr Color555 from_rgb(uint8_t r, uint8_t g, uint8_t b) {
        static constexpr auto to_5bit = [](uint8_t byte, uint8_t shift) {
            return ((byte >> 3) & 0b11111) << shift;
        };
        return Color555 {
            .raw = static_cast<uint16_t>(
                to_5bit(r, 0)
                | to_5bit(g, 5)
                | to_5bit(b, 10)
                //
                )
        };
    }

    constexpr Color to_color() const {
        const auto to_8bit = [this](uint8_t shift) -> uint8_t {
            // Standard way of casting 5-bit to 8-bit
            // Equals to round(v * 255 / 31) +- 1 (good enough, and fast)

            //     0 0 0 A B C D E
            const uint8_t v = (raw >> shift) & 0b11111;

            return
                // A B C D E 0 0 0
                (v << 3)
                // 0 0 0 0 0 A B C
                | (v >> 2);

            //  -> A B C D E A B C
        };
        return Color::from_rgb(to_8bit(0), to_8bit(5), to_8bit(10));
    }

    constexpr operator Color() const {
        return to_color();
    }

    constexpr bool operator==(const Color555 &) const = default;
};

static_assert(sizeof(Color555) == 2);

using OptionalColor555 = CompactOptional<Color555, Color555 { Color555::invalid_bit }>;

constexpr OptionalColor555 color555_from_raw(uint16_t raw) {
    return raw & Color555::invalid_bit ? OptionalColor555 {} : Color555 { .raw = raw };
}
