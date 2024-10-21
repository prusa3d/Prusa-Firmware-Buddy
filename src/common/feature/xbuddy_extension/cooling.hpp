#pragma once

#include <cstdint>
#include <optional>
#include <temperature.hpp>

namespace buddy {

/// Algorithm for controlling the cooling on the xbuddy extension.
class FanCooling {
private:
    /// Target-current temperature difference at which the fans go on full
    static constexpr int fans_max_temp_diff = 10;
    // We spin up the fans at the target temperature, but turn them off
    // when we get below the target temperature by this much.
    static constexpr int off_temp_below = 2;

    // Numbers pulled out of thin air
    static constexpr int max_pwm = 255;
    static constexpr int min_pwm = 70;
    static constexpr int spin_up_pwm = 200;

    void compute_auto(bool already_spinning, Temperature current_temperature);

public:
    bool auto_control = true;
    // Can be directly set if auto_control = false
    uint8_t target_pwm = 0;
    // The desired temperature; nullopt if no request (no cooling will happen in auto mode).
    std::optional<Temperature> target_temperature;

    // Compute at what PWM the fan(s) should be driven.
    uint8_t compute_pwm(bool already_spinning, Temperature current_temperature);
};

} // namespace buddy
