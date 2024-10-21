/**
 * @file power_check.hpp
 * @author Radek Vana
 * @brief Checks power consumption during selftest heater check
 * calculates with single heater bed or nozzle
 * @date 2021-11-12
 */
#pragma once

#include "selftest_heater_config.hpp"
#include "selftest_log.hpp"
#include <cstdint>

namespace selftest {

// Power check of a single heater element
//
// Holds runtime data needed to monitor the heating element.
// Instances are not reusable, runtime data is not reset. New instance needs
// to be created when the measurement is repeated.
class PowerCheck {
    static constexpr uint32_t log_minimal_delay = 1000;
    uint32_t last_pwm = 0;
    uint32_t start_stable_time_meas = 0;
    bool force_next_stable_log = true;
    // we need separate time stamps to ensure logs are not blocked by other logging
    LogTimer log_load;
    LogTimer log_status;

public:
    enum class status_t {
        stable, // evaluation time changed, measurement data valid
        unstable, // timeout did not end
        changed, // value just changed
        unmeasurable_pwm // pwm is too small to get reasonable value
    };
    enum class load_t {
        in_range,
        overload,
        underload,
        config_error
    };

    static constexpr const char *LoadTexts(load_t load) {
        switch (load) {
        case load_t::in_range:
            return "in range";
        case load_t::overload:
            return "overload";
        case load_t::underload:
            return "underload";
        case load_t::config_error:
            return "config error";
        }
        return "index out of range";
    }

    constexpr PowerCheck()
        : log_load(3000)
        , log_status(3000) {}

    status_t EvaluateHeaterStatus(uint32_t current_pwm, const HeaterConfig_t &config);

#if HAS_SELFTEST_POWER_CHECK()
    load_t EvaluateLoad(uint32_t current_pwm, float current_load_W, const HeaterConfig_t &config);
#endif
};
} // namespace selftest
