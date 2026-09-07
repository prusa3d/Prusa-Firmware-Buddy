/// @file
#include "heater_watch.hpp"

void HeaterWatch::arm(int16_t target_temp) {
    if (target_temp <= 0) {
        state_ = State::disarmed;
        return;
    }
    target_temp_celsius_ = target_temp;
    state_ = State::pending;
}

bool HeaterWatch::update(float current_temp) {
    switch (state_) {
    case State::disarmed:
        return false;

    case State::watching:
        if (!ELAPSED(millis(), next_check_ms_)) {
            return false;
        }
        if ((current_temp < baseline_threshold_celsius_) ^ config_.watch_cooling_instead) {
            // No re-arm - keeps firing every tick until the caller raises or resets via arm()
            return true;
        }
        // Period ended successfully — re-evaluate engage condition for the next one
        state_ = State::pending;
        [[fallthrough]];

    case State::pending:
        if (!(((target_temp_celsius_ - current_temp) > config_.min_temp_diff) ^ config_.watch_cooling_instead)) {
            state_ = State::disarmed;
            return false;
        }
        baseline_threshold_celsius_ = static_cast<int16_t>(current_temp) + config_.temp_increase;
        next_check_ms_ = millis() + config_.period_s * 1000UL;
        state_ = State::watching;
        return false;
    }

    return false;
}
