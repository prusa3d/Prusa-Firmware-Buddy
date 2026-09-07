/// \file
#pragma once

#include <algorithm>
#include <type_traits>
#include <bsod/bsod.h>

/// Utility class for making sure that something is not run too often
template <typename T_>
class RateLimiter {

public:
    using T = T_;
    using UT = std::make_unsigned_t<T>;

    // !!! Important! std::convertible_to<T> required to disable automatic type inferration from the min_delay type
    explicit RateLimiter(std::convertible_to<T> auto min_delay)
        // min_delay should never be < 0, so casting to unsigned is safe
        : min_delay_(static_cast<UT>(min_delay)) {
        debug_assert(min_delay >= 0);
    }

    void set_min_delay(UT set) {
        min_delay_ = set;
    }

    /// @returns last time where check(time) returned true, or 0
    T last_event() const {
        return last_event_;
    }

    /// Forget any previous events. Next event will not be limited
    void reset() {
        last_event_ = 0;
    }

    /// \returns true if we can perform an action (and also marks the current time as the last event)
    [[nodiscard]] bool check(T now) {
        // We need to work in unsigned to be able to cover overflows
        const UT diff = static_cast<UT>(now) - static_cast<UT>(last_event_);

        if (diff < min_delay_ && last_event_ != 0) {
            return false;
        }

        last_event_ = now;
        return true;
    }

    /// \returns how much time is remaining till we can run the event again
    [[nodiscard]] T remaining_cooldown(T now) const {
        if (last_event_ == 0) {
            return 0;
        }

        // We need to work in unsigned to be able to cover overflows
        const UT diff = static_cast<UT>(now) - static_cast<UT>(last_event_);
        return static_cast<T>(min_delay_ - std::min(diff, min_delay_));
    }

private:
    /// Timestamp of the last event; 0 = no event happened
    T last_event_ = 0;

    /// Minimum delay between two events
    UT min_delay_;
};
