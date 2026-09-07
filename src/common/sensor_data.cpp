#include <common/sensor_data.hpp>
#include <printers.h>

#include <option/has_ac_controller.h>
#include <option/has_bed_fan.h>
#include <option/has_door_sensor.h>
#include <option/has_mmu2.h>
#include <option/has_advanced_power.h>
#if HAS_ADVANCED_POWER()
    #include "advanced_power.hpp"
#endif // HAS_ADVANCED_POWER()

#include <adc.hpp>
#include "../Marlin/src/module/temperature.h"
#include <timing.h>
#include <raii/scope_guard.hpp>
#include <utils/math/ema.hpp>

#if HAS_AC_CONTROLLER()
    #include <puppies/ac_controller.hpp>
#endif
#if HAS_BED_FAN()
    #include <feature/bed_fan/bed_fan.hpp>
#endif
#if HAS_PSU_FAN()
    #include <feature/psu_fan/psu_fan.hpp>
#endif

#if BOARD_IS_XLBUDDY()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <fanctl.hpp>
#include <option/has_indx.h>
#if HAS_INDX()
    #include <puppies/INDX.hpp>
#endif

SensorData &sensor_data() {
    static SensorData instance;
    return instance;
}

SensorData::SensorData()
    : limiter(1000 / OVERSAMPLENR) {
}

void SensorData::update_MCU_temp() {
    if (limiter.check(ticks_ms())) {
        mcu_sum += AdcGet::getMCUTemp();

#if BOARD_IS_XLBUDDY()
        sandwich_sum += AdcGet::sandwichTemp();
        splitter_sum += AdcGet::splitterTemp();
#endif

        if (++sample_nr >= OVERSAMPLENR) {
            MCUTemp = static_cast<float>(mcu_sum) / OVERSAMPLENR;
            mcu_sum = 0;

#if BOARD_IS_XLBUDDY()
            sandwichTemp = Temperature::analog_to_celsius_board(sandwich_sum);
            splitterTemp = Temperature::analog_to_celsius_board(splitter_sum);
            sandwich_sum = 0;
            splitter_sum = 0;
#endif
            sample_nr = 0;
        }
    }
}

void SensorData::update() {
    const auto now_ms = ticks_ms();
    [[maybe_unused]] const uint32_t delta_time_ms = now_ms - last_update_ms_;

    ScopeGuard _sg = [&] { last_update_ms_ = now_ms; };

    // Board temperature
    boardTemp = thermalManager.degBoard();

    // MCU temperature
    update_MCU_temp();

#if HAS_DOOR_SENSOR()
    // Door sensor
    door_sensor_detailed_state = buddy::door_sensor().detailed_state();
#endif

    match(
        PhysicalToolIndex::currently_selected(), //
        [this](PhysicalToolIndex tool) {
            heatbreak_fan_pwm = Fans::heat_break(tool).get_pwm();
            //
        },
        [this](NoTool) {
            heatbreak_fan_pwm = 0;
            //
        });

#if BOARD_IS_XBUDDY()

    #if HAS_AC_CONTROLLER()
    bedMCUTemperature = buddy::puppies::ac_controller.get_mcu_temp().value_or(0.0f);
    bed_voltage = buddy::puppies::ac_controller.get_bed_voltage().value_or(0.0f);
    #else
    bed_voltage = advancedpower.bed_voltage();
    #endif
    heater_voltage = advancedpower.heater_voltage();
    heater_current = advancedpower.heater_current();
    input_current = advancedpower.input_current();

    #if HAS_MMU2()

    // MMU current
    mmuCurrent = advancedpower.GetMMUInputCurrent();

    #endif

#elif BOARD_IS_XLBUDDY()

    // Input voltage
    inputVoltage = advancedpower.Get24VVoltage();

    // Sandwich 5V voltage
    sandwich5VVoltage = advancedpower.Get5VVoltage();

    // Sandwich 5V current
    sandwich5VCurrent = advancedpower.GetDwarfSandwitch5VCurrent();

    // XLBUDDY board 5V current
    buddy5VCurrent = advancedpower.GetXLBuddy5VCurrent();

#endif
#if HAS_BED_FAN()
    if (auto rpm = bed_fan::bed_fan().get_rpm()) {
        bed_fan1_rpm = (*rpm)[0];
        bed_fan2_rpm = (*rpm)[1];
    } else {
        bed_fan1_rpm = 0;
        bed_fan2_rpm = 0;
    }
    bed_fan1_pwm = bed_fan::bed_fan().get_pwm().value_or(0);
    bed_fan2_pwm = bed_fan::bed_fan().get_pwm().value_or(0);
#endif
#if HAS_PSU_FAN()
    static_assert(HAS_AC_CONTROLLER());
    psu_fan_rpm = psu_fan::psu_fan().get_rpm().value_or(0);
    psu_fan_pwm = psu_fan::psu_fan().get_pwm().value_or(0);
#endif

#if HAS_INDX()
    // Calculate nozzle power
    if (delta_time_ms > 0) {
        const auto current_energy_uJ_ = buddy::puppies::indx.get_hotend_energy_consumed_uJ();

        const float spot_power_W = (current_energy_uJ_ - last_nozzle_energy_uJ_) / (float(delta_time_ms) * 1000) * buddy::puppies::Indx::hotend_induction_efficiency;

        // The current_energy_uJ_ gets updated from the indx_head in discrete steps, apply EMA to smoothen it out a bit
        nozzle_power_W_ = exponential_moving_average(nozzle_power_W_, spot_power_W, delta_time_ms, 400);

        last_nozzle_energy_uJ_ = current_energy_uJ_;
    }
#endif
}

#if HAS_DWARF()
float SensorData::head_pcb_temperature() {
    return prusa_toolchanger.getActiveToolOrFirst().get_board_temperature();
}
float SensorData::head_mcu_temperature() {
    return prusa_toolchanger.getActiveToolOrFirst().get_mcu_temperature();
}
#endif

#if HAS_INDX()
float SensorData::head_pcb_temperature() {
    return buddy::puppies::indx.get_board_temperature();
}
float SensorData::head_mcu_temperature() {
    return buddy::puppies::indx.get_mcu_temperature();
}
float SensorData::head_ambient_temperature() {
    return buddy::puppies::indx.get_tpis_ambient_temperature();
}
float SensorData::nozzle_temp_uncompensated() {
    return buddy::puppies::indx.get_hotend_temp_uncompensated();
}
int16_t SensorData::ringdown_decay() {
    return buddy::puppies::indx.get_ringdown_decay();
}
#endif
