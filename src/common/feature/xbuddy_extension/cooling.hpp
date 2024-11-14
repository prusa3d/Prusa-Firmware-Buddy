#pragma once

#include <cstdint>
#include <optional>
#include <temperature.hpp>

namespace buddy {

/// Algorithm for controlling the cooling on the xbuddy extension.
class FanCooling {
public:
    using FanPWM = uint8_t;

    /// Target-current temperature difference at which the fans go on full
    static constexpr Temperature fans_max_temp_diff = 10;

    // We spin up the fans at the target temperature, but turn them off
    // when we get below the target temperature by this much.
    static constexpr Temperature off_temp_below = 2;

    // Numbers pulled out of thin air
    static constexpr FanPWM max_pwm = 255;
    static constexpr FanPWM min_pwm = 40;
    static constexpr FanPWM spin_up_pwm = 100;

    bool auto_control = true;

    // Can be directly set if auto_control = false
    FanPWM target_pwm = 0;

    // The desired temperature; nullopt if no request (no cooling will happen in auto mode).
    std::optional<Temperature> target_temperature;

    // Compute at what PWM the fan(s) should be driven.
    FanPWM compute_pwm(bool already_spinning, Temperature current_temperature);

private:
    /// Computes a PWM ramping function
    static FanPWM compute_ramp(bool already_spinning, Temperature current_temperature, Temperature temp_ramp_start, Temperature temp_ramp_end, FanPWM max_pwm);
};

} // namespace buddy
