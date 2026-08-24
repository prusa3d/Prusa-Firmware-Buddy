/// @file

#include <tool/tool/standard_fff_physical_tool.hpp>
#include <tool/hotend/hotend/local_hotend.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <module/thermistor/thermistors.h>

PhysicalTool &PhysicalTool::for_index(PhysicalToolIndex) {
    static constexpr LocalHotend::Config hotend_config {
        .base_config {
            .min_nozzle_temp = 5,
            .max_nozzle_temp = 305,
        },
        .nozzle_temp_table = TT_NAME(THERMISTOR_HEATER_0),
        .heatbreak_temp_table = TT_NAME(TEMP_SENSOR_HEATBREAK),
        .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),

        // TODO: Set up HW PWM here? This is the only board that does not have it
        .nozzle_heater_soft_pwm = true,
    };
    static StandardFFFPhysicalTool<LocalHotend> tool { PhysicalToolIndex::from_raw(0), &hotend_config };
    static_assert(PhysicalToolIndex::count == 1);

    return tool;
}
