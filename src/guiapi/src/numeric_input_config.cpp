#include "numeric_input_config.hpp"

#include <math.h>

float NumericInputConfig::clamp(float value, float diff) const {
    const bool is_below_min = (value < min_value);

    if (!is_below_min && value <= max_value) {
        return value;

    } else if (special_value.has_value() && (special_value < max_value) == is_below_min && (diff == 0 || (diff < 0) == is_below_min)) {
        // Jump to special value if out of bounds and diff = 0 or diffing even more out of bounds
        return *special_value;

    } else {
        return is_below_min ? min_value : max_value;
    }
}

uint8_t NumericInputConfig::max_value_strlen(MaxValueStrlenArgs args) const {
    uint8_t r = std::max<uint8_t>(
        num_digits(abs(max_value)), //
        num_digits(abs(min_value)) + ((args.count_minus_sign && (min_value < 0)) ? 1 : 0) // Extra digit for minus
    );

    // Decimal point plus decimal places
    if (args.count_decimal_point && max_decimal_places > 0) {
        r += max_decimal_places + 1;
    }

    return r;
}
