#pragma once

#include <cstdint>

namespace indx_head::leds {
struct [[gnu::packed]] Color {
    uint8_t r {};
    uint8_t g {};
    uint8_t b {};

    constexpr bool operator==(const Color &) const = default;
};

enum class Mode : uint8_t {
    off,
    solid,
    blinking,
    pulsing,
    match_nozzle_temp,
};

struct [[gnu::packed]] alignas(uint16_t) LedConfig {
    Color primary;
    Color secondary;
    uint16_t delay_ms;
    Mode mode {};

    constexpr bool operator==(const LedConfig &) const = default;
};
} // namespace indx_head::leds
