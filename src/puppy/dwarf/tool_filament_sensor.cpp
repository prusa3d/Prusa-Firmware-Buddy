#include "tool_filament_sensor.hpp"
#include "filters/median_filter.hpp"
#include "adc.hpp"
#include <limits>

namespace dwarf::tool_filament_sensor {

static MedianFilter filter;
static int32_t fs_filtered_value;                                                           // current filtered value set from interrupt
inline constexpr int32_t fs_filtered_value_not_ready = std::numeric_limits<int32_t>::min(); // invalid value of fs_filtered_value
static int32_t raw_value = std::numeric_limits<int32_t>::min();

static uint8_t cnt_tool_filament_sensor_update = 0;
void tool_filament_sensor_irq() {
    if (++cnt_tool_filament_sensor_update >= 40) {
        raw_value = AdcGet::toolFimalentSensor();
        if (filter.filter(raw_value)) { // fs_raw_value is rewritten - passed by reference
            fs_filtered_value = raw_value;
        } else {
            fs_filtered_value = fs_filtered_value_not_ready;
        }
        cnt_tool_filament_sensor_update = 0;
    }
}

int32_t tool_filament_sensor_get_raw_data() {
    return fs_filtered_value;
}
}
