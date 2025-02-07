#include "cooling.hpp"

#include <algorithm>
#include <config_store/store_instance.hpp>

namespace buddy {

// Set maximum possiblem PWM to be used with automatic control
void FanCooling::set_soft_max_pwm(FanPWM val) {
    config_store().chamber_fan_max_control_pwm.set(val.value);
}

// Get maximum possiblem PWM to be used with automatic control
FanCooling::FanPWM FanCooling::get_soft_max_pwm() {
    return FanPWM { config_store().chamber_fan_max_control_pwm.get() };
}

FanCooling::FanPWM FanCooling::compute_auto_regulation_step(Temperature current_temperature, Temperature target_temperature) {
    FanPWM::Value desired = 0;

    const float error = current_temperature - target_temperature;

    // Simple P-regulation calculation
    float regulation_output = last_regulation_output + (proportional_constant * error);

    regulation_output = std::clamp<float>(regulation_output, 0.0f, static_cast<float>(get_soft_max_pwm().value));

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

FanCooling::FanPWM FanCooling::compute_pwm_step(Temperature current_temperature, std::optional<Temperature> target_temperature, FanPWMOrAuto target_pwm) {
    // Prevent cropping off 1 during the restaling
    FanPWM result = target_pwm.value_or(FanPWM { 0 });

    // Make sure the target_pwm contains the value we would _like_ to
    // run at.
    if (target_pwm == pwm_auto && target_temperature.has_value()) {
        result = compute_auto_regulation_step(current_temperature, *target_temperature);

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
