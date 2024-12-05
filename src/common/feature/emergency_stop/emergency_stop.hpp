#pragma once

#include <atomic>
#include <cstdint>

namespace buddy {

// Manage an emergency stop of the printer in case door is opened or under
// similar circumstances.
class EmergencyStop {
private:
    static constexpr int32_t no_emergency = std::numeric_limits<int32_t>::min();
    // Z position (in steps) at the start of current emergency, or no_emergency
    // if outside.
    //
    // Used to track we didn't move too far in Z during the emergency; in case
    // we do, we do some desperate measures to stop (power panic, BSOD...)
    //
    // In combination with the no_emergency, it is effectively an optional.
    // However, optionals aren't atomic and we want to access it from the ISR.
    std::atomic<int32_t> start_z = no_emergency;

    // Init before the start_z is set, so the ISR can just read them, without
    // accessing the config storage.
    std::atomic<int32_t> allowed_steps;
    std::atomic<int32_t> extra_emergency_steps;

    /// Stores whether the maybe_block() function is currently running
    bool maybe_block_running = false;

    /// A safety check for not allowing planning any movements during emergency
    bool allow_planning_movements = true;

    // Check the Z limits.
    //
    // This does more extensive handling than check_z_limits, but doesn't run
    // from ISR, therefore isn't guaranteed to run as soon as we would like it
    // to (though usually it does).
    void check_z_limits_soft();

    void invoke_emergency();

public:
    // Maximum "sizes" of a move segment.
    //
    // Used for limiting the sizes of segments submited to the planner,
    // allowing finer/sooner pause.
    //
    // Hopefully a compromise between being able to act fast and not spamming
    // the planner with too many too small segments.
    static constexpr float max_segment_time_s = 0.1;
    static constexpr float max_segment_z_mm = 0.05;
    // Check the z limits (how far did we move during an emergency situation).
    //
    // Can be called from ISR, should be called often enough.
    void check_z_limits();

    // To be called from loop, in userspace (in Marlin thread).
    void step();

    // Are we in emergency mode right now?
    bool in_emergency() const { return start_z != no_emergency; }

    /// Checks for emergency and if there is one, blocks until it is over (also possibly parking the toolhead).
    /// This is intended to be called on various places in the marlin task, where want to prevent the user from taking action
    /// until the emergency is over
    void maybe_block();

    /// To be called from within Planner.buffer_segment. BSODs if the movement planning is not allowed
    void assert_can_plan_movement();
};

EmergencyStop &emergency_stop();

} // namespace buddy
