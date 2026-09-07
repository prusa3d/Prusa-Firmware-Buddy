/// @file
#pragma once

#include <cstdint>
#include <pwm_utils.hpp>

namespace buddy {

/// Off below temp_on_c, then a linear ramp min_pwm..max_pwm reached at
/// temp_full_c. The on/off threshold has hysteresis so the fan does not chatter
/// when the integer MCU temperature dithers around temp_on_c.
class ModularBedFanControl {
public:
    static constexpr int temp_on_c = 50; ///< [°C] at/above: fan runs
    static constexpr int temp_off_c = 48; ///< [°C] below: a running fan stops (hysteresis)
    static constexpr int temp_full_c = 65; ///< [°C] at/above: run at max_pwm
    static constexpr uint8_t min_pwm = 76; ///< ~30 % of 255; the fan may not spin below this
    static constexpr uint8_t max_pwm = 153; ///< 60 % of 255

    static ModularBedFanControl &instance();

    [[nodiscard]] PWM255OrAuto pwm_override() const { return pwm_override_; }

    /// Manual PWM override (M106)
    void set_pwm_override(PWM255OrAuto target);

    /// Fan duty (0-255) for the current MB MCU temperature [°C]. Advances the
    /// hysteresis state even while overridden, so the automatic output is
    /// current the moment the override is released.
    [[nodiscard]] uint8_t update(int mcu_temperature_c);

private:
    // Singleton in firmware; the unit test constructs its own instances to keep
    // cases independent, so the constructor stays public there.
#ifndef UNITTESTS
    ModularBedFanControl() = default;
#endif

    PWM255OrAuto pwm_override_ = pwm_auto;
    bool running_ = false;
};

} // namespace buddy
