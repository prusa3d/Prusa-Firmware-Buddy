#pragma once

#include <cstdlib>
#include <stdint.h>
#include <array>

namespace dwarf::tool_filament_sensor {
void tool_filament_sensor_irq();
uint16_t tool_filament_sensor_get_filtered_data();
} // namespace dwarf::tool_filament_sensor
