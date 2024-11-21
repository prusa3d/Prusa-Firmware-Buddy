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

    // Is the gcode to block the print in queue / active anywhere?
    bool gcode_scheduled = false;

    void emergency_start();
    void emergency_over();

public:
    // Check the z limits (how far did we move during an emergency situation).
    //
    // Can be called from ISR, should be called often enough.
    void check_z_limits();
    // To be called from loop, in userspace (in Marlin thread).
    void step();
    // Are we in emergency mode right now?
    bool in_emergency() const { return start_z != no_emergency; }

    // The actual "implementation" of the pause during print behavior, as
    // called from the relevant gcode. Parks and waits for the emergency to
    // pass.
    void gcode_body();
};

EmergencyStop &emergency_stop();

} // namespace buddy
