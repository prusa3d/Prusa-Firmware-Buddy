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

public:
    std::atomic<bool> do_stop = false;
    void step();
    // Should be used only from the Marlin thread.
    static bool stop_scheduled;
};

EmergencyStop &emergency_stop();

} // namespace buddy
