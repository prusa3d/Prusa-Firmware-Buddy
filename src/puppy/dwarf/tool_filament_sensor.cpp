#include "tool_filament_sensor.hpp"
#include "filters/median_filter.hpp"
#include "adc.hpp"
#include <limits>

namespace dwarf::tool_filament_sensor {

static MedianFilter filter;
static uint16_t fs_filtered_value = AdcGet::undefined_value; // current filtered value set from interrupt
static uint16_t raw_value = AdcGet::undefined_value;

static uint8_t cnt_tool_filament_sensor_update = 0;
void tool_filament_sensor_irq() {
    if (++cnt_tool_filament_sensor_update >= 40) {
        raw_value = AdcGet::toolFimalentSensor();

        // copy and filter the raw value (rewritten by reference) just to match the MedianFilter
        // type. The value is kept unchanged if the filter is not ready, meaning
        // AdcGet::undefined_value is already kept during startup as expected.
        int32_t raw_value_copy = raw_value;
        filter.filter(raw_value_copy);
        fs_filtered_value = raw_value_copy;

        cnt_tool_filament_sensor_update = 0;
    }
}

uint16_t tool_filament_sensor_get_filtered_data() {
    return fs_filtered_value;
}
} // namespace dwarf::tool_filament_sensor
