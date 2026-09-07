#pragma once

#include <option/has_cpu_fan.h>

#if HAS_CPU_FAN()

    #include <algorithm>
    #include <cmath>
    #include <cstdint>
    #include <pwm_utils.hpp>
    #include <CFanCtlCommonConsts.hpp>

namespace cpu_fan_controller {

// Hysteretic on/off thresholds (°C). The JDL3006S operating range is
// 4.5-5.5 V, so FANCTLCPU_PWM_THR is the lowest duty that keeps it spinning.
inline constexpr float temp_off = 55.0f;
inline constexpr float temp_on = 65.0f;
inline constexpr float temp_full = 80.0f;
inline constexpr uint16_t pct_step = 10; // PWM change step (and hysteresis) in percent

static_assert(temp_off < temp_on && temp_on < temp_full, "temperature thresholds are invalid");

constexpr uint16_t compute_pwm(float temp_c, uint16_t current_pwm) {
    // Fail-safe: default ON when sensor data is unreliable.
    if (!std::isfinite(temp_c)) {
        return static_cast<uint16_t>(255.f * FANCTLCPU_PWM_MAX / 100.f);
    }

    if (temp_c >= temp_full) {
        // Full speed above this threshold
        return static_cast<uint16_t>(FANCTLCPU_PWM_MAX * 255.f / 100.f);
    } else if (temp_c >= temp_on) {
        // Above this temperature calculate the duty cycle percentage based on the temperature
        // Duty cycle is changed in steps pct_step, which is also creating a hysteresis effect
        uint16_t pct_increment = static_cast<uint16_t>((temp_c - temp_on) * (FANCTLCPU_PWM_MAX - FANCTLCPU_PWM_THR) * 255.f / (temp_full - temp_on) / 100.f);

        if (std::abs(current_pwm - pct_increment) >= pct_step) {
            return static_cast<uint16_t>(FANCTLCPU_PWM_THR * 255.f / 100.f) + pct_increment;
        }
    } else if (temp_c <= temp_off) {
        // Below this temperature, turn the fan off
        return 0;
    }

    return current_pwm;
}

/// Update CPU fan speed based on temperature
void update(float temp_c);

/// Manual PWM override (M106 P7), or pwm_auto for temperature-driven
/// control. A manual value is applied immediately and reasserted by
/// update(); it persists until pwm_auto is set again, or reboot.
/// Marlin task only, like update().
void set_target_pwm(PWM255OrAuto target);

} // namespace cpu_fan_controller

#endif // HAS_CPU_FAN()
