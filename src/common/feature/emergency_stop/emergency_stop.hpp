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

    void emergency_start();
    void emergency_over();
    
    enum class MaybeBlockState {
        not_running,
        running,
        executing_move,
    };
    MaybeBlockState maybe_block_state = MaybeBlockState::not_running;

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
};

EmergencyStop &emergency_stop();

} // namespace buddy
