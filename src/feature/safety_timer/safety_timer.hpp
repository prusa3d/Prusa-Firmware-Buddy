/// @file
#pragma once

#include <cstdint>
#include <array>

#include <utils/uncopyable.hpp>
#include <utils/timing/timer.hpp>
#include <option/has_human_interactions.h>
#include <inc/MarlinConfig.h>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

namespace buddy {

/// Class responsible for disabling the heaters when the printer is inactive for some amount of time
class SafetyTimer {
    friend SafetyTimer &safety_timer();
    friend class SafetyTimerBlocker;
    friend class SafetyTimerNonBlockingGuard;

public:
    using Time = uint32_t;
    using NozzleTargetTemperatures = StrongIndexArray<int16_t, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static>;

    enum class State : uint8_t {
        /// Either not expired or expired without the ability to restore (= outside of print)
        idle,

        /// The safety timer is triggered and is holding the temperatures to be restored
        active,

        /// Restoring the temperatures back to the normal
        restoring,
    };

#if !HAS_HUMAN_INTERACTIONS()
    static constexpr Time default_interval = 10 * 60 * 1000;
#else
    static constexpr Time default_interval = 30 * 60 * 1000;
#endif

public:
    inline Time interval() const {
        return activity_timer_.interval();
    }

    void set_interval(Time set);

public:
    inline State state() const {
        return state_;
    }

    inline bool is_active() const {
        return state_ == State::active;
    }

    /// @returns "original" target hotend temperatures (that is what the temperatures would be without the SafetyTimer being active).
    /// If the SafetyTimer is not active, returns current targets.
    NozzleTargetTemperatures original_hotend_targets() const;

public:
    /// Restarts the inactivity timer, but if the timer is already triggered, it will NOT restore the temperatures
    void reset_norestore();

    /// RESTART the inactivity timer and RESTORE the temperatures (if the timer was holding)
    /// Restoring the temperatures would be NONBLOCKING
    /// The target temperatures will be set, but the function will not wait for them to be reached
    void reset_restore_nonblocking();

    /// RESTARTS the inactivity timer and BLOCKINGLY RESTORES the temperatures (if the timer was holding)
    void reset_restore_blocking();

    /// Trigger safety timer, disable all heaters and whatnot
    /// Will throw a BSOD if a SafetyTimerBlocker is active
    void trigger();

public:
    /// Needs to be called periodically to make things work
    void step();

private:
    SafetyTimer() = default;

private:
    State state_ = State::idle;

    /// Number of currently active SafetyTimerBlocker guards
    uint8_t blocker_count_ = 0;

    /// Number of currently active SafetyTimerNonBlocking guards
    uint8_t non_blocking_guard_count_ = 0;

    /// Timestamp of the last activity
    utils::Timer<Time> activity_timer_ { default_interval };

    /// Target nozzle temperatures to restore
    NozzleTargetTemperatures nozzle_temperatures_to_restore_;

    /// Recursion prevention mechanism
    bool prevent_recursion_ = false;
};

/// Guard marking a section where safety timer will not expire
/// The guard does NOT call \p reset_restore_blocking()
class SafetyTimerBlocker : public Uncopyable {

public:
    SafetyTimerBlocker();
    ~SafetyTimerBlocker();
};

/// Guard marking a section that will turn all reset_restore blocking calls to nonblocking ones
/// As a result, moves planned within this guard will go through even when the temperatures are not restoring yet
class SafetyTimerNonBlockingGuard : public Uncopyable {

public:
    SafetyTimerNonBlockingGuard();
    ~SafetyTimerNonBlockingGuard();
};

/// @returns instance to the SafetyTimer
/// !!! ONLY to be accessed from the defaultTask
SafetyTimer &safety_timer();

} // namespace buddy
