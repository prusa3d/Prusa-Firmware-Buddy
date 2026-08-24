/// @file

#include <tool/hotend/hotend/local_hotend.hpp>
#include <tool/tool/standard_fff_physical_tool.hpp>
#include <utils/storage/strong_index_array.hpp>

PhysicalTool &PhysicalTool::for_index(PhysicalToolIndex) {
    static constexpr LocalHotend::Config hotend_config {
        .base_config {
            .min_nozzle_temp = 10,
            .max_nozzle_temp = 295,
        },
        .nozzle_temp_table = TT_NAME(THERMISTOR_HEATER_0),
        .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),

        // Note: This was true before the hotends refactoring,
        // but Mini actually has HW PWM support for the heater
        .nozzle_heater_soft_pwm = false,
    };
    static StandardFFFPhysicalTool<LocalHotend> tool { PhysicalToolIndex::from_raw(0), &hotend_config };
    static_assert(PhysicalToolIndex::count == 1);

    return tool;
}
