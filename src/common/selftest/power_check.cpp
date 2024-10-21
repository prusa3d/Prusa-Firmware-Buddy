/**
 * @file power_check.cpp
 * @author Radek Vana
 * @date 2021-11-12
 */

#include "power_check.hpp"
#include "i_selftest.hpp" //SelftestInstance().GetTime();
#include "selftest_log.hpp"

LOG_COMPONENT_REF(Selftest);
using namespace selftest;
// PWM must be stable for n cycles
PowerCheck::status_t PowerCheck::EvaluateHeaterStatus(uint32_t current_pwm, const HeaterConfig_t &config) {
    uint32_t now = SelftestInstance().GetTime();
    if (current_pwm < config.min_pwm_to_measure) {
        last_pwm = current_pwm;
        start_stable_time_meas = now;
        LogDebugTimed(log_status, "%s pwm too small to measure load", config.partname);
        return status_t::unmeasurable_pwm;
    }
    if (current_pwm > config.pwm_100percent_equivalent_value) {
        last_pwm = current_pwm;
        start_stable_time_meas = now;
        log_error(Selftest, "%s pwm out of range", config.partname);
        return status_t::unmeasurable_pwm;
    }
    if (std::abs(static_cast<int>(current_pwm) - static_cast<int>(last_pwm)) > config.heater_load_stable_difference) {
        last_pwm = current_pwm;
        start_stable_time_meas = now;
        LogDebugTimed(log_status, "%s pwm changed", config.partname);
        return status_t::changed;
    }
    if ((now - start_stable_time_meas) < config.heater_load_stable_ms) {
        LogDebugTimed(log_status, "%s pwm stabilizing", config.partname);
        force_next_stable_log = true;
        return status_t::unstable;
    }

    // cannot loose first stable log
    if (force_next_stable_log) {
        force_next_stable_log = false;
        log_status.ForceNextLog();
    }
    LogInfoTimed(log_status, "%s pwm stable", config.partname);
    return status_t::stable;
}

#if HAS_SELFTEST_POWER_CHECK()
PowerCheck::load_t PowerCheck::EvaluateLoad(uint32_t current_pwm, float current_load_W, const HeaterConfig_t &config) {
    if (config.heater_full_load_min_W > config.heater_full_load_max_W) {
        log_error(Selftest, "%s invalid config file, max value of full load must be greater than min", config.partname);
        return load_t::config_error;
    }
    if (config.pwm_100percent_equivalent_value == 0) {
        log_error(Selftest, "%s invalid config file, 100%% pwm cannot be 0", config.partname);
        return load_t::config_error;
    }
    if (config.min_pwm_to_measure == 0) {
        log_error(Selftest, "%s invalid config file, 0%% pwm cannot be measured", config.partname);
        return load_t::config_error;
    }
    const float scaled_load_min = config.heater_full_load_min_W * current_pwm / config.pwm_100percent_equivalent_value;
    const float scaled_load_max = config.heater_full_load_max_W * current_pwm / config.pwm_100percent_equivalent_value;
    LogDebugTimed(log_load, "%s load: %f, scaled min load: %f, scaled max load: %f", config.partname, double(current_load_W), double(scaled_load_min), double(scaled_load_max));
    if (current_load_W < scaled_load_min) {
        log_error(Selftest, "%s measured load is too small", config.partname);
        return load_t::underload;
    }
    if (current_load_W > scaled_load_max) {
        log_error(Selftest, "%s measured load is too big", config.partname);
        return load_t::overload;
    }
    return load_t::in_range;
}
#endif
