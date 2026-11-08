/// @file
#pragma once

#include <cstdint>

namespace fanctl {

/// PWM period in 1 kHz ticks that dynamic PWM (CFanCtl3WireDynamic) uses for a fan PWM of 1-255.
/// A low PWM gets a long period, so that each pulse is long enough to drive the fan.
constexpr uint8_t dynamic_pwm_period(uint8_t pwm) {
    if (pwm < 50) {
        return 250;
    } else if (pwm < 80) {
        return 125;
    } else if (pwm < 120) {
        return 50;
    } else {
        return 40;
    }
}

} // namespace fanctl
