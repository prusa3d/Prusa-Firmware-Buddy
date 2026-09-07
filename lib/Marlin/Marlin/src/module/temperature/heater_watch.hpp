/// @file
#pragma once

#include <cstdint>

#include <core/millis_t.h>
#include <wiring_time.h>
#include <module/temperature/temp_defines.hpp>

/// Sanity-checks that a heater makes progress toward its target temperature.
///
/// arm() records the target as intent; the baseline is captured on the
/// next update() call. This decouples target arming from baseline data,
/// so arming can happen synchronously (e.g. from start_heating()) before
/// the next manage tick refreshes the current temperature.
class HeaterWatch {
public:
    struct Config {
        /// By how many degrees must the temperature increase
        /// within the @p period
        /// for the watch to not trigger
        int16_t temp_increase;

        /// The period for the temp to increase, in seconds
        uint16_t period_s;

        /// Minimum target-current temperature difference for the watch to engage
        int16_t min_temp_diff;

        /// Inverts the checking logic
        /// !!! If true, temp_increase and min_temp_diff should probably be negative
        bool watch_cooling_instead = false;
    };

    explicit HeaterWatch(const Config &config)
        : config_(config) {}

    /// Set the watch target. Baseline is captured by the next update().
    /// target_temp <= 0 disarms.
    void arm(int16_t target_temp);

    /// Tick: captures baseline when pending, checks progress when the period elapses.
    /// @returns true when the watch fired; the caller is expected to raise its error
    [[nodiscard]] bool update(float current_temp);

    /// @returns true when armed (pending or watching).
    bool is_running() const {
        return state_ != State::disarmed;
    }

private:
    enum class State : uint8_t {
        disarmed,
        pending,
        watching,
    };

    const Config config_;
    State state_ = State::disarmed;
    int16_t target_temp_celsius_;
    int16_t baseline_threshold_celsius_;
    millis_t next_check_ms_;
};
