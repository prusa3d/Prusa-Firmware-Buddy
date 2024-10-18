#pragma once

#include <cstdint>
#include <optional>
#include <temperature.hpp>

namespace buddy {

/// Algorithm for controlling the cooling on the xbuddy extension.
class Cooling {
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

public:
    bool auto_control = true;
    // Can be directly set if auto_control = false
    uint8_t require_pwm = 0;
    // The desired temperature; nullopt if no request (no cooling will happen in auto mode).
    std::optional<Temperature> target;

    uint8_t pwm(bool already_spinning, Temperature temp) {
        // Phase 1: Decide what PWM we would like to run at
        if (auto_control && !target.has_value()) {
            // No temp set -> don't cool.
            require_pwm = 0;
        } else if (auto_control) {
            // Linear mapping from the allowed temp difference over the target to allowed RPM range
            const int temp_diff = temp - *target;
            const int pwm_diff = max_pwm - min_pwm;
            int desired = temp_diff * pwm_diff / fans_max_temp_diff + min_pwm;

            if (desired > max_pwm) {
                // Don't go over max.
                require_pwm = max_pwm;
            } else if (desired < min_pwm) {
                const bool below_target_by_much = *target - off_temp_below > temp;
                if (already_spinning && !below_target_by_much) {
                    // If it is already spinning, we keep it spinning until it
                    // falls below the target by the margin, so we don't go
                    // on-off-on-off too much.
                    require_pwm = min_pwm;
                } else {
                    require_pwm = 0;
                }
            } else {
                require_pwm = desired;
            }
        } // else -> no auto_control -> keep require_pwm intact

        // Phase 2: adjust to minima and spinning up.

        // If the fans are not spinning yet and should be, give them a bit of a
        // kick to get turning. Unfortunately, we can't do that to each of them
        // individually, they share the PWM, even though they have separate RPM
        // measurement.
        if (!already_spinning && require_pwm > 0 && require_pwm < spin_up_pwm) {
            return spin_up_pwm;
        }

        if (require_pwm > 0 && require_pwm < min_pwm) {
            // Even if the user sets it to some low %, keep them at least on the
            // minimum (the auto thing never sets it between 0 and min, so it's
            // only in the manual case).
            return min_pwm;
        }

        return require_pwm;
    }
};

} // namespace buddy
