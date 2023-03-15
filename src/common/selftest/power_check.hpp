/**
 * @file power_check.hpp
 * @author Radek Vana
 * @brief Checks power consumption during selftest heater check
 * calculates with single heater bed or nozzle
 * @date 2021-11-12
 */

#include "selftest_heater.h"
#include "selftest_log.hpp"
#include <cstdint>

namespace selftest {

class PowerCheck {
    static constexpr uint32_t log_minimal_delay = 1000;
    CSelftestPart_Heater *htr;
    uint32_t last_pwm;
    uint32_t start_stable_time_meas;
    bool force_next_stable_log;
    // we need separate time stamps to ensure logs are not blocked by other logging
    LogTimer log_load;
    LogTimer log_status;

public:
    enum class status_t {
        stable,          // evaluation time changed, measurement data valid
        unstable,        // timeout did not end
        changed,         // value just changed
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
        : htr(nullptr)
        , last_pwm(0)
        , start_stable_time_meas(0)
        , force_next_stable_log(true)
        , log_load(3000)
        , log_status(3000) {}

    status_t EvaluateHeaterStatus(uint32_t current_pwm);
    load_t EvaluateLoad(uint32_t current_pwm, float current_load_W);

    constexpr void Bind(CSelftestPart_Heater &f) {
        htr = &f;
        last_pwm = 0;
        start_stable_time_meas = 0;
    }
    constexpr void UnBind() { htr = nullptr; }
    constexpr bool IsActive() { return htr != nullptr; };
    void Fail() { htr->state_machine.Fail(); };
};
}
