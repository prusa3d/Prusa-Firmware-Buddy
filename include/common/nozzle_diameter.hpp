#pragma once

#include <numeric_input_config.hpp>

static constexpr NumericInputConfig nozzle_diameter_spin_config {
    .min_value = 0.2,
    .max_value = 1.8,
    .step = 0.05,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};
