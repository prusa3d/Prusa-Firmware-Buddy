#pragma once

#include <cstdint>
#include <optional>
#include <temperature.hpp>
#include <cmath>

#include <pwm_utils.hpp>

namespace buddy {

/// Algorithm for controlling the cooling on the xbuddy extension.
class FanCooling {
public:
    using FanPWM = PWM255;
    using FanPWMOrAuto = PWM255OrAuto;

    /// Temperature at which the fans start spinning at full speed, no matter what
    static constexpr Temperature overheating_temp = 70.0f;

    /// Temperature at which the print is stopped and fans at full power, not matter what
    static constexpr Temperature critical_temp = 75.0f;

    /// Temperature at which the normal fan control may be restored, after emergency temperature
    static constexpr Temperature recovery_temp = 60.0f;

    // Numbers pulled out of thin air
    static constexpr FanPWM max_pwm { 255 };
    static constexpr FanPWM min_pwm { 40 };
    static constexpr FanPWM spin_up_pwm { 100 };

    // time step for regulation loop
    static constexpr float dt_s = 1.0f;

    static constexpr float proportional_constant = 1.5f * dt_s;

    /// Applies spinup and emergency fan overrides
    [[nodiscard]] FanPWM apply_pwm_overrides(bool already_spinning, FanPWM pwm) const;

    // Compute at what PWM the fan(s) should be driven
    // !!!!!!!! this function should be called in regular time intervals given by dt_s !!!!!!!!
    [[nodiscard]] FanPWM compute_pwm_step(Temperature current_temperature, std::optional<Temperature> target_temperature, FanPWMOrAuto target_pwm);

    constexpr bool get_overheating_temp_flag() { return overheating_temp_flag; };
    constexpr bool get_critical_temp_flag() { return critical_temp_flag; };

    static void set_soft_max_pwm(FanPWM val);
    static FanPWM get_soft_max_pwm();

private:
    /// Computes a PWM ramping function
    FanPWM compute_auto_regulation_step(Temperature current_temperature, Temperature target_temperature);
    float last_regulation_output = 0.0f;

    bool overheating_temp_flag = false;
    bool critical_temp_flag = false;
};

} // namespace buddy
