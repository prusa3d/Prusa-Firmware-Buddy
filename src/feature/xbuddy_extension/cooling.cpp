#include "cooling.hpp"

#include <algorithm>
#include <feature/chamber_filtration/chamber_filtration.hpp>
#include <bsod/bsod.h>

namespace buddy {

FanCooling::FanPWM FanCooling::compute_auto_regulation_step(Temperature current_temperature, Temperature target_temperature, FanPWM max_auto_pwm) {
    /**
     * #### XBuddyExtension Chamber Fan Auto Control Logic (fans 3 & 4 on CORE ONE printers)
     * For compatibility reasons, two chamber fan regulators are implemented. New algorithm is more oriented on airflow stability,
     * while legacy algorithm is more oriented on absolute temperature control. On print start and end, the regulator is switched
     * to legacy mode to keep compatibility with old gcodes. When M106 with parameters N or G is sent, the regulator is switched to new algorithm.
     *
     * ##### New algorithm
     * - Chamber Fan control algorithm is ramp function with hysteresis on top of it
     * - If temperature is below target, ramp function output is ramp_breakpoint_pwm (Parameter N)
     *   The minimal PWM is to ensure good airflow to cool the extruded material fast enough, which is necessary even when the chamber is on the target temperature.
     *   This minimal required airflow is material specific, and thus it has been exposed to be made configurable via gcode.
     *
     * - If temperature is above target, ramp function output is proportional to the error with slope ramp_slope (Parameter G)
     * - Hysteresis is applied on top of the ramp function to avoid fan premature kick-start and reduce kick-starts frequency
     * - The PWM output is also modified based on the filtration backend to adjust for different fan configurations
     *
     * ##### Legacy algorithm
     * - Legacy Fan control algorithm is PID regulator with only I component used.
     *
     * !! This comment is also doubled in GcodeSuite::M106. If you do changes here, update the other one, too.
     */

    if (target_temperature == 0.0f) {
        return FanPWM { 0 };
    }

    const auto filtration_backend = chamber_filtration().backend();
    float ramp_mult = 1.0f; // Parameter multipliers for filters.
    switch (filtration_backend) {
    case ChamberFiltrationBackend::xbe_official_filter:
        ramp_mult = 3.0f;
        break;
    case ChamberFiltrationBackend::xbe_filter_on_cooling_fans:
        ramp_mult = 2.0f;
        break;
    case ChamberFiltrationBackend::none:
        // no multipliers for no filtration
        break;
    default:
        bsod_unreachable();
        break;
    }

    const float error = current_temperature - target_temperature;
    float target_pwm = (std::max(error, 0.0f) * ramp_slope + ramp_breakpoint_pwm) * ramp_mult;

    static constexpr float hysteresis_l = 26.0f; // [0-255] PWM duty cycle - below this, turn off
    static constexpr float hysteresis_h = 38.0f; // [0-255] PWM duty cycle - above this, turn on
    if (target_pwm < hysteresis_l) {
        target_pwm = 0;
    } else if (target_pwm < hysteresis_h && last_regulation_output == 0) {
        target_pwm = 0;
    }

    target_pwm = std::min<float>(max_auto_pwm.value, target_pwm);

    last_regulation_output = target_pwm;
    FanPWM::Value desired = static_cast<FanPWM::Value>(target_pwm);
    return FanPWM { desired };
}

FanCooling::FanPWM FanCooling::compute_auto_regulation_step_legacy(Temperature current_temperature, Temperature target_temperature, FanPWM max_auto_pwm) {
    FanPWM::Value desired = 0;

    const float error = current_temperature - target_temperature;

    // Simple Feedback Regulator calculation - PID regulator, but only Integration part is used
    float regulation_output = last_regulation_output + (integration_constant * error);

    regulation_output = std::clamp<float>(regulation_output, 0.0f, static_cast<float>(max_auto_pwm.value));

    // convert float result to integer, while keeping the range of float for next loop
    desired = static_cast<FanPWM::Value>(regulation_output);
    last_regulation_output = regulation_output;

    return FanPWM { desired };
}

FanCooling::FanPWM FanCooling::apply_pwm_overrides(bool already_spinning, FanPWM pwm) const {
    if (overheating_temp_flag || critical_temp_flag) {
        // Max cooling after temperature overshoot
        return max_pwm;
    }

    if (pwm.value == 0) {
        return pwm;
    }

    // If the fans are not spinning yet and should be, give them a bit of a
    // kick to get turning. Unfortunately, we can't do that to each of them
    // individually, they share the PWM, even though they have separate RPM
    // measurement.
    if (!already_spinning) {
        return std::max(pwm, spin_up_pwm);
    }

    // Even if the user sets it to some low %, keep them at least on the
    // minimum (the auto thing never sets it between 0 and min, so it's
    // only in the manual case).
    return std::max(pwm, min_pwm);
}

FanCooling::FanPWM FanCooling::compute_pwm_step(Temperature current_temperature, std::optional<Temperature> target_temperature, FanPWMOrAuto target_pwm, FanPWM max_auto_pwm) {
    // Prevent cropping off 1 during the restaling
    FanPWM result = target_pwm.value_or(FanPWM { 0 });

    // Make sure the target_pwm contains the value we would _like_ to
    // run at.
    if (target_pwm == pwm_auto && target_temperature.has_value()) {
        if (regulator_legacy) { // We need to keep legacy regulator for old gcode compatibility
            result = compute_auto_regulation_step_legacy(current_temperature, *target_temperature, max_auto_pwm);
        } else {
            result = compute_auto_regulation_step(current_temperature, *target_temperature, max_auto_pwm);
        }

    } else {
        // Reset regulator if we lose the control
        last_regulation_output = 0.0f;
    }

    if (current_temperature >= critical_temp) {
        // Critical cooling - overrides any other control, goes at full power
        critical_temp_flag = true;

    } else if (current_temperature >= overheating_temp) {
        // Emergency cooling - overrides other control, goes at full power
        overheating_temp_flag = true;

    } else if (current_temperature < recovery_temp) {
        overheating_temp_flag = false;
        critical_temp_flag = false;
    }

    return result;
}

} // namespace buddy
