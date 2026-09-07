#include "modular_bed_fan.hpp"

#include <algorithm>

namespace buddy {

ModularBedFanControl &ModularBedFanControl::instance() {
    static ModularBedFanControl instance;
    return instance;
}

[[nodiscard]] uint8_t ModularBedFanControl::update(int mcu_temperature_c) {
    if (running_) {
        running_ = mcu_temperature_c >= temp_off_c;
    } else {
        running_ = mcu_temperature_c >= temp_on_c;
    }

    if (pwm_override_.has_value()) {
        return pwm_override_->value;
    }

    if (!running_) {
        return 0;
    }
    if (mcu_temperature_c >= temp_full_c) {
        return max_pwm;
    }
    // Clamp the ramp's lower end so it yields min_pwm in the still-running
    // hysteresis band (temp_off_c..temp_on_c) instead of dropping below it.
    const int t = std::max(mcu_temperature_c, temp_on_c);
    return static_cast<uint8_t>(min_pwm + (t - temp_on_c) * (max_pwm - min_pwm) / (temp_full_c - temp_on_c));
}

void ModularBedFanControl::set_pwm_override(PWM255OrAuto target) {
    pwm_override_ = target;
}

} // namespace buddy
