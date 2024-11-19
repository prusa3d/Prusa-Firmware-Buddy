#pragma once

#include <atomic>
#include <cstdint>
#include <optional>

namespace buddy {

// Manage an emergency stop of the printer in case door is opened or under
// similar circumstances.
class EmergencyStop {
private:
    std::optional<int32_t> start_z;
    // Is the gcode to block the print in queue / active anywhere?
    bool gcode_scheduled = false;
    bool active = false;

    void emergency_start();
    void emergency_over();

public:
    void step();
    bool in_emergency() const { return active; }

    // The actual "implementation" of the pause during print behavior, as
    // called from the relevant gcode. Parks and waits for the emergency to
    // pass.
    void gcode_body();
};

EmergencyStop &emergency_stop();

} // namespace buddy
