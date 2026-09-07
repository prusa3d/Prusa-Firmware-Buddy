/// @file
#pragma once

#include <tool_index.hpp>
#include <utils/publisher.hpp>

/// Speeds up nozzle cooling by running the print & heatbreak fans at full power.
///
/// While active, the fans of every enabled tool whose nozzle is hotter than
/// `target_temp` run at full power and return to normal control once the nozzle
/// cools down to the target (or on deactivation/destruction). Updates itself on
/// the marlin idle loop. Owns the fans' selftest mode while active, so it must
/// not be used concurrently with the fan selftest.
class AllToolsQuickCoolingManager {
public:
    explicit AllToolsQuickCoolingManager(float target_temp);

    ~AllToolsQuickCoolingManager() {
        set_active(false);
    }

    /// Enable/disable the cooldown; the effect on the fans is applied immediately
    void set_active(bool set);

private:
    /// Applies the desired fan state (called from set_active and on marlin idle)
    void step();

    float target_temp;
    bool active = false;
    Subscriber<> idle_hook;
};
