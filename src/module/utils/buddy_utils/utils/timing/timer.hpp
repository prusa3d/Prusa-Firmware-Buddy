/// @file
#pragma once

#include <concepts>
#include <type_traits>
#include <cstdint>
#include <bsod/bsod.h>

namespace utils {

enum class TimerState : uint8_t {
    /// Timer has been stopped/hasn't been started yet
    idle,

    /// Timer is active
    /// !!! The state of the timer needs to be updated by calling check()
    running,

    /// Timer has finished/expired
    finished,
};

/// Utility class for making sure that something is not run too often
template <typename T_>
class Timer {
    static_assert(std::is_unsigned_v<T_>, "No signed types - overflows are UB");

public:
    using T = T_;
    using State = TimerState;

    // !!! Important! std::convertible_to<T> required to disable automatic type inferration from the interval type
    explicit Timer(std::convertible_to<T> auto interval)
        : interval_(interval) {
        debug_assert(interval > 0);
    }

    inline T interval() const {
        return interval_;
    }

    inline void set_interval(T set) {
        debug_assert(set > 0);
        interval_ = set;
    }

    /// @returns whether the timer has been started and is still running
    /// !!! The state of the timer needs to be updated by calling check()
    inline State state() const {
        return state_;
    }

    inline bool is_running() const {
        return state_ == State::running;
    }

    /// @returns elapsed time since the timer was started (or 0 if the timer is not running)
    inline T elapsed(T now) const {
        return is_running() ? (now - start_time_) : 0;
    }

    void stop() {
        state_ = State::idle;
    }

    void restart(T now) {
        state_ = State::running;
        start_time_ = now;
    }

    /// Updates the timer state
    /// @returns true if the timer just expired
    [[nodiscard]] bool check(T now) {
        if (elapsed(now) < interval_) {
            // Either not running nor not yet expired
            return false;
        }

        // Just expired
        state_ = State::finished;
        return true;
    }

private:
    /// Timestamp of when the timer started
    T start_time_ = 0;

    /// Time  after which timer expires
    T interval_;

    State state_ = State::idle;
};

} // namespace utils
