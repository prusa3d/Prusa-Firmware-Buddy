#include "numeric_input_config.hpp"

#include <math.h>

float NumericInputConfig::clamp(float value) const {
    const bool is_below_min = (value < min_value);

    if (!is_below_min && value <= max_value) {
        return value;

    } else if (special_value.has_value() && (special_value < max_value) == is_below_min) {
        return *special_value;

    } else {
        return is_below_min ? min_value : max_value;
    }
}

uint8_t NumericInputConfig::max_value_strlen() const {
    static constexpr auto num_digits = [](float v) {
        return std::max<float>(floor(log10(abs(v))), 0) + 1;
    };

    uint8_t r = std::max(
        num_digits(max_value), //
        num_digits(min_value) + (min_value < 0 ? 1 : 0) // Extra digit for minus
    );

    // Decimal point plus decimal places
    if (max_decimal_places > 0) {
        r += max_decimal_places + 1;
    }

    return r;
}
