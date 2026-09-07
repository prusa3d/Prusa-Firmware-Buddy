#include "motion.hpp"

#include <Marlin/src/module/planner.h>

#include <option/has_remote_accelerometer.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#if HAS_REMOTE_ACCELEROMETER() && HAS_TOOLCHANGER()
    #include <module/tool_change.h>
#endif

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_filament_tracker.h>
#if HAS_FILAMENT_TRACKER()
    #include <feature/filament_tracker/filament_tracker.hpp>
#endif

#include <raii/auto_restore.hpp>
#include "src/module/motion.h"

namespace mapi {

bool extruder_move(float distance, float feed_rate, bool ignore_flow_factor) {
    // Dry run - only simulate extruder moves
    if (DEBUGGING(DRYRUN)) {
        return true;
    }

    // We cannot work with current_position, because current_position might or might not have MBL applied on the Z axis.
    // So we gotta use planner.position_float, which should always be matching.
    auto pos = planner.position_float;
    pos.e += distance;

    // ! Imporant - do not use buffer_line, it would reapply modifiers on top of the position_float
    const auto result = planner.buffer_segment(pos, feed_rate, PhysicalToolIndex::currently_selected(), PlannerHints { .move { .ignore_e_factor = ignore_flow_factor } });

    // But we gotta update current_position.e, too. .e should be always the same with planner.position_float (hopefully).
    // Only .z should ever differ because of MBL application.
    current_position.e = pos.e;

    return result;
}

float extruder_schedule_turning(float feed_rate, float step) {
    if (planner.movesplanned() <= 3) {
        extruder_move(feed_rate > 0 ? step : -step, std::abs(feed_rate));
        return step;
    }

    return 0;
}

void fully_deretract([[maybe_unused]] float fr_mm_s) {
    [[maybe_unused]] const auto tool = PhysicalToolIndex::currently_selected_opt();
    if (!tool.has_value()) {
        return;
    }

#if HAS_AUTO_RETRACT()
    // First, deretract auto-retract
    // Auto-retract might not be stored in the filament_tracker,
    // beause auto_retract is persistent and filament_tracker is runtime only
    // Plus it inserts deretract moves automatically on extrusion,
    // so if we don't take it off before querying the filament_tracker,
    // we could end up deretracting the distance twice.
    buddy::auto_retract().maybe_deretract_to_nozzle();
#endif

#if HAS_FILAMENT_TRACKER()
    mapi::extruder_move(buddy::filament_tracker().get_retracted_distance(*tool).value_or(0), fr_mm_s);
#endif

    planner.synchronize();
}

namespace {

    /// Moves the extruder so the filament of the currently selected tool ends up retracted
    /// by \p target_retraction_distance.
    void move_to_retracted_distance(float target_retraction_distance, float fr_mm_s, bool allow_deretract) {
        [[maybe_unused]] const auto tool = PhysicalToolIndex::currently_selected_opt();
        if (!tool.has_value()) {
            return;
        }

        float current_retraction_distance = 0;

#if HAS_AUTO_RETRACT()
        const auto auto_retraction_distance = buddy::auto_retract().retracted_distance(*tool).value_or(0);
        if (!allow_deretract && (auto_retraction_distance >= target_retraction_distance)) {
            // Already retracted enough, quit
            return;

        } else if (auto_retraction_distance > 0) {
            // "Take" the retracted distance out from auto_retract
            // Retractions are blocked if auto_retract is active

            current_retraction_distance = auto_retraction_distance;
            buddy::auto_retract().set_retracted_distance(*tool, std::nullopt);
        }
#endif

#if HAS_FILAMENT_TRACKER()
        // Only resort to filament tracker if we were not auto_retracted
        // auto_retract data is persistent across restarts, so it has higher priority
        // and while auto_retracted, no extruder moves are permitted, so the value should be exact
        if (current_retraction_distance == 0) {
            current_retraction_distance = buddy::filament_tracker().get_retracted_distance(*tool).value_or(0);
        }
#endif

        const float delta = target_retraction_distance - current_retraction_distance;
        if (allow_deretract ? (std::abs(delta) > 1e-6f) : (delta > 1e-6f)) {
            mapi::extruder_move(-delta, fr_mm_s);
        }

        planner.synchronize();
    }

} // namespace

void retract_to(float target_retraction_distance, float fr_mm_s) {
    move_to_retracted_distance(target_retraction_distance, fr_mm_s, false);
}

void restore_retracted_distance(float target_retraction_distance, float fr_mm_s) {
    move_to_retracted_distance(target_retraction_distance, fr_mm_s, true);
}

void ensure_tool_with_accelerometer_picked() {
#if HAS_REMOTE_ACCELEROMETER() && HAS_TOOLCHANGER()
    if (std::holds_alternative<NoTool>(PhysicalToolIndex::currently_selected())) {
        if (!prusa_toolchanger.pick_any_tool(tool_return_t::no_return, {}, tool_change_lift_t::no_lift, false)) {
            fatal_error("No calibrated dock", "PrusaToolChanger");
        }
    }
#endif
}

} // namespace mapi
