#pragma once

#include <cstdint>
#include <optional>
#include <temperature.hpp>

namespace buddy {

/// Algorithm for controlling the cooling on the xbuddy extension.
class FanCooling {
public:
    using FanPWM = uint8_t;

    /// Temperature at which the fans start spinning at full speed, no matter what
    static constexpr Temperature overheating_temp = 70.0f;

    /// Temperature at which the print is stopped and fans at full power, not matter what
    static constexpr Temperature critical_temp = 75.0f;

    /// Temperature at which the normal fan control may be restored, after emergency temperature
    static constexpr Temperature recovery_temp = 60.0f;

    /// Maximum PWM for user and automatic control
    static constexpr FanPWM soft_max_pwm = 100;

    // Numbers pulled out of thin air
    static constexpr FanPWM max_pwm = 255;
    static constexpr FanPWM min_pwm = 40;
    static constexpr FanPWM spin_up_pwm = 100;

    // time step for regulation loop
    static constexpr float dt_s = 1.0f;

    static constexpr float proportional_constant = 1.5f * dt_s;

    // Can be directly set if auto_control = false
    FanPWM target_pwm = 0;

    // The desired temperature; nullopt if no request (no cooling will happen in auto mode).
    std::optional<Temperature> target_temperature;

    // Compute at what PWM the fan(s) should be driven
    // !!!!!!!! this function should be called in regular time intervals given by dt_s !!!!!!!!
    FanPWM compute_pwm_step(bool already_spinning, Temperature current_temperature);

    /// @returns percentage 0-100% converted from PWM value (0-max_pwm)
    /// @note For now PWM range is 0-max_pwm. In the future, other ranges may appear if HW changes.
    /// The idea here is to have a set of conversion routines at one spot to allow future changes.
    static constexpr uint8_t pwm2pct(uint8_t pwm) {
        return static_cast<uint8_t>(((uint16_t)pwm) * 100U / max_pwm);
    }

    /// @returns PWM value (0-max_pwm) from percentage 0-100%
    static constexpr uint8_t pct2pwm(uint8_t pct) {
        return static_cast<uint8_t>(((uint16_t)pct) * max_pwm / 100U);
    }

    constexpr bool get_overheating_temp_flag() { return overheating_temp_flag; };
    constexpr bool get_critical_temp_flag() { return critical_temp_flag; };

    void set_auto_control(bool ac);
    constexpr bool get_auto_control() const { return auto_control; };

private:
    bool auto_control = true;
    /// Computes a PWM ramping function
    FanPWM compute_auto_regulation_step(Temperature current_temperature);
    float last_regulation_output = 0.0f;

    bool overheating_temp_flag = false;
    bool critical_temp_flag = false;
};

} // namespace buddy
