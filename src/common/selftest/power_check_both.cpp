/**
 * @file power_check_both.cpp
 * @author Radek Vana
 * @date 2021-11-12
 */

#include "power_check_both.hpp"
#include "selftest_log.hpp"
#include "advanced_power.hpp"
#include "../../Marlin/src/module/temperature.h"

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

/**
 * @brief callback function to be called periodically
 * Bed does not have independed current measurement and we cannot substract currents,
 * because load is switched inductance
 * So calculations are done with watage
 *
 * Nozzle PWM must be stable for n cycles to measure nozzle power
 * Both PWM must be stable for n cycles to measure bed power
 */
void PowerCheckBoth::Callback() {
#if (!PRINTER_IS_PRUSA_XL)
    const float total_current_A = advancedpower.GetInputCurrent();
    const float nozzle_current_A = advancedpower.GetHeaterCurrent();
    const float nozzle_voltage_V = advancedpower.GetHeaterVoltage();
    const float bed_voltage_V = advancedpower.GetBedVoltage();
#else
    // TODO: Fake implementation to enable XL selftest before it is ready
    const float total_current_A = 42;  // advancedpower.GetInputCurrent();
    const float nozzle_current_A = 42; // advancedpower.GetHeaterCurrent();
    const float nozzle_voltage_V = 42; // advancedpower.GetHeaterVoltage();
    const float bed_voltage_V = 42;    // advancedpower.GetBedVoltage();
#endif

    const float total_power_W = bed_voltage_V * total_current_A;
    const float nozzle_power_W = nozzle_voltage_V * nozzle_current_A;
    const float bed_power_W = total_power_W - nozzle_power_W;

    const uint32_t noz_pwm = thermalManager.temp_hotend[0].soft_pwm_amount;
    const uint32_t bed_pwm = thermalManager.temp_bed.soft_pwm_amount;

    // Nozzle has independed load measuring
    if (noz.IsActive()) {
        if (noz.EvaluateHeaterStatus(noz_pwm) != PowerCheck::status_t::stable) {
            return; // nozzle not sable, bed measuring would be distorted
        }
        LogDebugTimed(log_noz, "Noz %fV, %fA, %fW, pwm %i", double(nozzle_voltage_V), double(nozzle_current_A), double(nozzle_power_W), noz_pwm);
        PowerCheck::load_t result = noz.EvaluateLoad(noz_pwm, nozzle_power_W);
        if (result != PowerCheck::load_t::in_range) {
            noz.Fail();
            log_error(Selftest, "Nozzle %s.", PowerCheck::LoadTexts(result));
            // cannot calculate bed, Fail it too
            if (bed.IsActive()) {
                bed.Fail();
            }
        }
    }

    // Bed only - depended load measuring, can measure only when nozzle is stable too
    if (bed.IsActive()) {
        if (bed.EvaluateHeaterStatus(bed_pwm) == PowerCheck::status_t::stable) {
            LogDebugTimed(log_bed, "Bed %fV, %fW, pwm %i, total current %fA", double(bed_voltage_V), double(bed_power_W), bed_pwm, double(total_current_A));
            PowerCheck::load_t result = bed.EvaluateLoad(bed_pwm, bed_power_W);
            if (result != PowerCheck::load_t::in_range) {
                bed.Fail();
                log_error(Selftest, "Bed %s.", PowerCheck::LoadTexts(result));
            }
        }
    }
}
