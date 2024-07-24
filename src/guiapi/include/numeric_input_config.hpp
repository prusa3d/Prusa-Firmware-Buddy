#pragma once

#include <stddef.h>
#include <optional>
#include <i18n.h>
#include <units.hpp>
#include <cmath>

// Had to put this outside because of a gcc bug
struct NumericInputConfig_MaxValueStrlenArgs {
    bool count_decimal_point = true;
    bool count_minus_sign = true;
};

struct NumericInputConfig {

public:
    /// Minimum allowed value
    float min_value = 0;

    /// Maximum allowed value
    float max_value;

    /// Increase/decrease step, used for spin edits
    float step = 1;

    /// Enables special value - should be outside of min/max value or on the border, on either side
    std::optional<float> special_value = std::nullopt;

    /// Display string of the nullopt value
    const char *special_value_str = N_("Off");

    /// How many decimal places the input has
    uint8_t max_decimal_places = 0;

    /// Unit, appended after the value
    Unit unit = Unit::none;

public:
    /// Clamps the value to min/max or sets it to special value if out of bounds
    /// If \param diff sign matches the direction the value is out of bounds, does not to special value, but clamps to the bounds
    float clamp(float value, float diff = 0) const;

    /// \returns number of digits of the provided value (excluding decimal ones)
    static constexpr uint8_t num_digits(uint32_t value) {
        return std::max<float>(floor(log10(value)), 0) + 1;
    }

    using MaxValueStrlenArgs = NumericInputConfig_MaxValueStrlenArgs;

    /// \returns maximum string length of a value
    uint8_t max_value_strlen(MaxValueStrlenArgs args = {}) const;

    /// \returns string of the unit
    string_view_utf8 unit_str() const {
        return string_view_utf8::MakeCPUFLASH(units_str[unit]);
    }
};

/// Similar to monostate
struct SpecialNumericValue {};
