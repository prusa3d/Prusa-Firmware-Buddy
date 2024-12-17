#include "cooling.hpp"

#include <algorithm>

namespace buddy {

FanCooling::FanPWM FanCooling::compute_ramp(bool already_spinning, Temperature current_temperature, Temperature temp_ramp_start, Temperature temp_ramp_end, FanPWM max_pwm) {
    // Linear mapping from the allowed temp difference over the target to allowed RPM range
    const Temperature temp_diff = current_temperature - temp_ramp_start;
    const FanPWM desired = static_cast<FanPWM>(std::clamp<int>(temp_diff * max_pwm / (temp_ramp_end - temp_ramp_start), 0, max_pwm));

    // If the fan is already spinning, we keep it spinning until it
    // falls below the target by the margin, so we don't go
    // on-off-on-off too much.
    if (desired == 0 && already_spinning && current_temperature >= temp_ramp_start - off_temp_below) {
        return 1;
    }

    return desired;
}

FanCooling::FanPWM FanCooling::compute_pwm(bool already_spinning, Temperature current_temperature) {
    // Make sure the target_pwm contains the value we would _like_ to
    // run at.
    if (auto_control && target_temperature) {
        target_pwm = compute_ramp(already_spinning, current_temperature, *target_temperature, *target_temperature + fans_max_temp_diff, soft_max_pwm);

    } else if (auto_control) {
        target_pwm = 0;

    } // else -> leave as is

    // Prevent cropping off 1 during the restaling
    FanPWM result = target_pwm;

    // Emergency cooling - overrides any other control, goes at full power
    const FanPWM emergency_cooling = compute_ramp(already_spinning, current_temperature, emergency_cooling_temp - fans_max_temp_diff, emergency_cooling_temp, max_pwm);
    result = std::max(result, emergency_cooling);

    if (result == 0) {
        return 0;
    }

    // If the fans are not spinning yet and should be, give them a bit of a
    // kick to get turning. Unfortunately, we can't do that to each of them
    // individually, they share the PWM, even though they have separate RPM
    // measurement.
    if (!already_spinning) {
        result = std::max(result, spin_up_pwm);
    }

    // Even if the user sets it to some low %, keep them at least on the
    // minimum (the auto thing never sets it between 0 and min, so it's
    // only in the manual case).
    result = std::max(result, min_pwm);

    return result;
}

} // namespace buddy
