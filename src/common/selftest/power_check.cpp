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
PowerCheck::status_t PowerCheck::EvaluateHeaterStatus(uint32_t current_pwm) {
    if (current_pwm < htr->m_config.min_pwm_to_measure) {
        LogDebugTimed(log_status, "%s pwm too small to measure load", htr->m_config.partname);
        return status_t::unmeasurable_pwm;
    }
    if (current_pwm > htr->m_config.pwm_100percent_equivalent_value) {
        log_error(Selftest, "%s pwm out of range", htr->m_config.partname);
        return status_t::unmeasurable_pwm;
    }
    uint32_t now = SelftestInstance().GetTime();
    if (current_pwm != last_pwm) {
        last_pwm = current_pwm;
        start_stable_time_meas = now;
        LogDebugTimed(log_status, "%s pwm changed", htr->m_config.partname);
        return status_t::changed;
    }
    if ((now - start_stable_time_meas) < htr->m_config.heater_load_stable_ms) {
        LogDebugTimed(log_status, "%s pwm stabilizing", htr->m_config.partname);
        force_next_stable_log = true;
        return status_t::unstable;
    }

    // cannot loose first stable log
    if (force_next_stable_log) {
        force_next_stable_log = false;
        log_status.ForceNextLog();
    }
    LogInfoTimed(log_status, "%s pwm stable", htr->m_config.partname);
    return status_t::stable;
}

PowerCheck::load_t PowerCheck::EvaluateLoad(uint32_t current_pwm, float current_load_W) {
    if (htr->m_config.heater_full_load_min_W > htr->m_config.heater_full_load_max_W) {
        log_error(Selftest, "%s invalid config file, max value of full load must be greater than min", htr->m_config.partname);
        return load_t::config_error;
    }
    if (htr->m_config.pwm_100percent_equivalent_value == 0) {
        log_error(Selftest, "%s invalid config file, 100% pwm cannot be 0", htr->m_config.partname);
        return load_t::config_error;
    }
    if (htr->m_config.min_pwm_to_measure == 0) {
        log_error(Selftest, "%s invalid config file, 0 pwm cannot be measured", htr->m_config.partname);
        return load_t::config_error;
    }
    const float scaled_load_min = htr->m_config.heater_full_load_min_W * current_pwm / htr->m_config.pwm_100percent_equivalent_value;
    const float scaled_load_max = htr->m_config.heater_full_load_max_W * current_pwm / htr->m_config.pwm_100percent_equivalent_value;
    LogDebugTimed(log_load, "%s load: %f, scaled min load: %f, scaled max load: %f", htr->m_config.partname, double(current_load_W), double(scaled_load_min), double(scaled_load_max));
    if (current_load_W < scaled_load_min) {
        log_error(Selftest, "%s measured load is too small", htr->m_config.partname);
        return load_t::underload;
    }
    if (current_load_W > scaled_load_max) {
        log_error(Selftest, "%s measured load is too big", htr->m_config.partname);
        return load_t::overload;
    }
    return load_t::in_range;
}
