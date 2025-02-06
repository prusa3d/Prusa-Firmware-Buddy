#include "cooling.hpp"

#include <algorithm>
#include <config_store/store_instance.hpp>

namespace buddy {

// Set maximum possiblem PWM to be used with automatic control
void FanCooling::set_soft_max_pwm(FanPWM val) {
    config_store().chamber_fan_max_control_pwm.set(val);
}

// Get maximum possiblem PWM to be used with automatic control
FanCooling::FanPWM FanCooling::get_soft_max_pwm() {
    return config_store().chamber_fan_max_control_pwm.get();
}

FanCooling::FanPWM FanCooling::compute_auto_regulation_step(Temperature current_temperature) {
    FanPWM desired = 0;

    const float error = current_temperature - *target_temperature;

    // Simple P-regulation calculation
    float regulation_output = last_regulation_output + (proportional_constant * error);

    regulation_output = std::clamp<float>(regulation_output, 0.0f, static_cast<float>(get_soft_max_pwm()));

    // convert float result to integer, while keeping the range of float for next loop
    desired = static_cast<FanPWM>(regulation_output);
    last_regulation_output = regulation_output;

    return desired;
}

FanCooling::FanPWM FanCooling::compute_pwm_step(bool already_spinning, Temperature current_temperature) {
    // Make sure the target_pwm contains the value we would _like_ to
    // run at.
    if (auto_control && target_temperature) {
        target_pwm = compute_auto_regulation_step(current_temperature);
    } else if (auto_control) {
        target_pwm = 0;

    } // else -> leave as is

    // Prevent cropping off 1 during the restaling
    FanPWM result = target_pwm;

    if (current_temperature >= critical_temp) {
        // Critical cooling - overrides any other control, goes at full power
        critical_temp_flag = true;
    } else if ((current_temperature >= overheating_temp) && (!critical_temp_flag)) {
        // Emergency cooling - overrides other control, goes at full power
        overheating_temp_flag = true;
    }

    if (overheating_temp_flag || critical_temp_flag) {
        // Max cooling after temperature overshoot
        result = max_pwm;
        if (current_temperature < recovery_temp) {
            overheating_temp_flag = false;
            critical_temp_flag = false;
        }
    } else if (result > 0) {
        if (!already_spinning) {
            // If the fans are not spinning yet and should be, give them a bit of a
            // kick to get turning. Unfortunately, we can't do that to each of them
            // individually, they share the PWM, even though they have separate RPM
            // measurement.
            result = std::max(result, spin_up_pwm);
        } else {
            // Even if the user sets it to some low %, keep them at least on the
            // minimum (the auto thing never sets it between 0 and min, so it's
            // only in the manual case).
            result = std::max(result, min_pwm);
        }
    }
    return result;
}

void FanCooling::set_auto_control(bool ac) {
    if (ac && !auto_control) {
        // reset regulator only if auto_control is set true for the first time
        target_pwm = 0;
        last_regulation_output = 0.0f;
    }
    auto_control = ac;
}

} // namespace buddy
