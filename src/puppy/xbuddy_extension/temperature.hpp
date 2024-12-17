/// @file
#pragma once

#include <cstdint>

namespace temperature {

/**
 * Convert raw ADC reading from the thermistor to temperature in Celsius.
 */
float raw_to_celsius(uint16_t raw);

} // namespace temperature
