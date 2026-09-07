/// @file
#include "indx_tool_lock_hack.hpp"

#include <module/planner.h>
#include <module/prusa/toolchanger.h>

namespace buddy {

INDXToolLockHack &indx_tool_lock_hack() {
    static INDXToolLockHack instance;
    return instance;
}

INDXToolLockHack::INDXToolLockHack() {
    // We don't remember how things were the last print run, assume we need the hack
    rearm();
}

void INDXToolLockHack::rearm() {
    extrusion_needed_mm_ = 1.2f;
}

void INDXToolLockHack::track_extruder_move(float delta_e, Badge<Planner>) {
    if (!is_armed()) {
        return;
    }

    if (delta_e >= 0) {
        // We are extruding - going towards the disarming
        // just track the move
        extrusion_needed_mm_ -= delta_e;

    } else {
        // We're trying to retract
        // Inject the deretract in this case

        const auto old_pos = planner.position_float;

        const auto tool = PhysicalToolIndex::currently_selected();

        const PlannerHints hints {
            .move {
                // If we can retract, assume we can also deretract
                .extrusion_safety_checks = false,
                .ignore_e_factor = true,
                .is_service_extruder_move = true,
            },
        };

        // We don't need to worry about recursion - these are marked as service moves,
        // track_extruder_move is not called for them
        planner.buffer_segment(old_pos + xyze_pos_t { .e = extrusion_needed_mm_ }, PrusaToolChanger::E_LOCK_FEEDRATE, tool, hints);
        planner.buffer_segment(old_pos, PrusaToolChanger::E_FULL_OPEN_FEEDRATE, tool, hints);

        extrusion_needed_mm_ = 0;

        // No need for any sync_position, we've returned to the original location
    }
}

} // namespace buddy
