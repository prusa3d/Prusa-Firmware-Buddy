#include "power_check_both.hpp"
#include "selftest_log.hpp"
#include "advanced_power.hpp"
#include "../../Marlin/src/module/temperature.h"
#include "selftest_heater.h"

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

/**
 * @brief callback function to be called periodically
 * Bed does not have independent current measurement and we cannot subtract currents,
 * because load is switched inductance
 * So calculations are done with wattage
 *
 * Nozzle PWM must be stable for n cycles to measure nozzle power
 * Both PWM must be stable for n cycles to measure bed power
 */
void PowerCheckBoth::Callback([[maybe_unused]] CSelftestPart_Heater &part) {

#if HAS_TOOLCHANGER() && HAS_MODULARBED()
    const float nozzle_current_A = advancedpower.get_nozzle_current(part.m_config.tool_nr);
    const float nozzle_voltage_V = advancedpower.get_nozzle_voltage(part.m_config.tool_nr);
    const uint32_t nozzle_pwm = advancedpower.get_nozzle_pwm(part.m_config.tool_nr);
    const float nozzle_power_W = nozzle_voltage_V * nozzle_current_A;
#else
    const float nozzle_current_A = advancedpower.GetHeaterCurrent();
    const float nozzle_voltage_V = advancedpower.GetHeaterVoltage();
    const uint32_t nozzle_pwm = thermalManager.temp_hotend[0].soft_pwm_amount;
    const float nozzle_power_W = nozzle_voltage_V * nozzle_current_A;
#endif

#if HAS_MODULARBED()
    const float bed_voltage_V = 24; // Modular bed does not measure this
    const float bed_current_A = advancedpower.get_bed_current();
    const float bed_power_W = bed_current_A * bed_voltage_V;
    [[maybe_unused]] const float total_current_A = nozzle_current_A + bed_current_A;
#else
    const float bed_voltage_V = advancedpower.GetBedVoltage();
    [[maybe_unused]] const float total_current_A = advancedpower.GetInputCurrent();
    const float total_power_W = bed_voltage_V * total_current_A;
    const float bed_power_W = total_power_W - nozzle_power_W;

#endif

    const uint32_t bed_pwm = thermalManager.temp_bed.soft_pwm_amount;

    // Nozzle has independent load measuring
    if (nozzle) {
        if (nozzle->check.EvaluateHeaterStatus(nozzle_pwm, nozzle->m_config) != PowerCheck::status_t::stable) {
            return; // nozzle not stable, bed measuring would be distorted
        }
        LogDebugTimed(nozzle->check_log, "Noz %fV, %fA, %fW, pwm %" PRIu32, static_cast<double>(nozzle_voltage_V),
            static_cast<double>(nozzle_current_A), static_cast<double>(nozzle_power_W), nozzle_pwm);

        PowerCheck::load_t result = nozzle->check.EvaluateLoad(nozzle_pwm, nozzle_power_W, nozzle->m_config);
        if (result != PowerCheck::load_t::in_range) {
            nozzle->state_machine.Fail();
            log_error(Selftest, "Nozzle %s.", PowerCheck::LoadTexts(result));
            // cannot calculate bed, Fail it too
            if (bed) {
                bed->state_machine.Fail();
            }
            nozzle->power_check_passed = true;
        }
    }

    // Bed only - dependent load measuring, can measure only when nozzle is stable too
    if (bed) {
        if (bed->check.EvaluateHeaterStatus(bed_pwm, bed->m_config) == PowerCheck::status_t::stable) {
            LogDebugTimed(bed->check_log, "Bed %fV, %fW, pwm %" PRIu32 ", total current %fA", static_cast<double>(bed_voltage_V),
                static_cast<double>(bed_power_W), bed_pwm, static_cast<double>(total_current_A));
            PowerCheck::load_t result = bed->check.EvaluateLoad(bed_pwm, bed_power_W, bed->m_config);
            if (result != PowerCheck::load_t::in_range) {
                bed->state_machine.Fail();
                log_error(Selftest, "Bed %s.", PowerCheck::LoadTexts(result));
            }
            bed->power_check_passed = true;
        }
    }
}
