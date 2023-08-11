#pragma once

#include <cstdlib>
#include <stdint.h>
#include <array>

#include "loadcell_shared.hpp"

namespace dwarf::tool_filament_sensor {
void tool_filament_sensor_irq();
int32_t tool_filament_sensor_get_raw_data();
} // namespace dwarf::tool_filament_sensor
