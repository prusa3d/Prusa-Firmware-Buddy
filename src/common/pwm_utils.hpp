#pragma once

#include <cstdint>
#include <expected>
#include <algorithm>
#include <cmath>

/// Strong class for representing a PWM value
template <typename Value_, Value_ max_>
struct PWMValue {
    using Value = Value_;
    static constexpr Value max = max_;

    Value value = 0;

    constexpr PWMValue() {}
    constexpr PWMValue(const PWMValue &o) = default;

    explicit constexpr PWMValue(Value value)
        : value(value) {}

    constexpr int8_t to_percent() const {
        return static_cast<uint8_t>(std::round((value * 100.f) / max));
    }
    static constexpr PWMValue from_percent(int8_t percent) {
        return PWMValue { static_cast<Value>(std::round((std::clamp<int8_t>(percent, 0, 100) * max) / 100.f)) };
    }

    constexpr auto operator<=>(const PWMValue &) const = default;
};

struct PWMAuto {
    constexpr bool operator==(const PWMAuto &) const = default;
};

constexpr std::unexpected pwm_auto { PWMAuto {} };

using PWM255 = PWMValue<uint8_t, 255>;
using PWM255OrAuto = std::expected<PWM255, PWMAuto>;
