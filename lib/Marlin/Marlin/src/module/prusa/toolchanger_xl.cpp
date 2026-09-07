#include "toolchanger.h"
#include "module/planner.h"
#include "module/tool_change.h"
#include "utils/overloaded_visitor.hpp"

#include <option/has_crash_detection.h>
#include <option/has_toolchanger.h>
#include <bsod/bsod.h>
#if HAS_TOOLCHANGER()
    #include "Marlin/src/module/stepper.h"
    #include "Marlin/src/module/motion.h"
    #include "Marlin/src/feature/bedlevel/bedlevel.h"
    #include "Marlin/src/gcode/gcode.h"
    #include <logging/log.hpp>
    #include "timing.h"
    #include "fanctl.hpp"
    #include <marlin_server.hpp>
    #include <cmath_ext.h>
    #include <odometer.hpp>
    #include <pause_stubbed.hpp>
    #include "module/temperature.h" // for fan control
    #include <mapi/motion.hpp>
    #include <mapi/feedrates/standard_feedrates.hpp>
    #include <raii/scope_guard.hpp>
    #include <feature/gcode_exception/gcode_exception.hpp>

    #if HAS_CRASH_DETECTION()
        #include "../../feature/prusa/crash_recovery.hpp"
    #endif

    #if DISABLED(ARC_SUPPORT)
        #error "toolchanger requires ARC_SUPPORT"
    #endif

    #include <option/has_auto_retract.h>
    #if HAS_AUTO_RETRACT()
        #include <feature/auto_retract/auto_retract.hpp>
    #endif

    #include <option/has_nozzle_thermal_compensation.h>
    #if HAS_NOZZLE_THERMAL_COMPENSATION()
        #include <feature/nozzle_thermal_compensation/nozzle_thermal_compensation.hpp>
    #endif

LOG_COMPONENT_REF(PrusaToolChanger);

PrusaToolChanger prusa_toolchanger;

using namespace buddy::puppies;

namespace arc_move {

// generated arc parameters
constexpr float arc_max_radius = 75.f; // mm
constexpr float arc_min_radius = 10.f; // mm
constexpr float arc_tg_jerk = 20.f; // mm/s
constexpr bool arc_backtravel_allow = true;
constexpr float arc_backtravel_max = 2.f; // 1/ratio

/**
 * @brief Calculate the tangent arc radius
 * @param pos Starting position
 * @param target Dock position
 * @return Arc radius (mm)
 */
static float arc_radius(const xy_pos_t &pos, const xy_pos_t &target) {
    float arc_d_y = fabsf(target.y - pos.y);
    float r = min(hypotf(target.x - pos.x, arc_d_y) * 0.5f, arc_max_radius);

    if constexpr (!arc_backtravel_allow) {
        // do not allow the Y carriage to move backwards
        return min(r, arc_d_y);
    } else {
        // limit the backtravel r
        return min(r, arc_d_y * arc_backtravel_max);
    }
}

/**
 * @brief Calculate the arc tangent point and angle parameters
 * @param arc_r Arc radius (mm)
 * @param pos Starting position
 * @param target Dock position
 * @param arc_x Output: Calculated arc center X position
 * @param point Output: Tangent point
 */
static void tangent_point(const float arc_r, const xy_pos_t &pos, const xy_pos_t &target,
    float &arc_x, xy_pos_t &point) {

    float arc_dir = pos.x < target.x ? 1 : -1;
    arc_x = target.x - arc_r * arc_dir;
    float arc_d_x = (pos.x - arc_x);
    float arc_d_y = (pos.y - target.y);
    float d = hypotf(arc_d_x, arc_d_y);
    float h = sqrtf(SQR(d) - SQR(arc_r));
    float arc_a = atan2f(arc_d_y, arc_d_x) + atanf(h / arc_r) * arc_dir;
    point.x = arc_x + cosf(arc_a) * arc_r;
    point.y = target.y + sinf(arc_a) * arc_r;
}

/**
 * @brief Calculate arc move
 * @param dest Destination XY position
 * @param center Rotation center
 * @param fr Target feedrate
 */
static void plan_arc2(const xy_pos_t &dest, const xy_pos_t &center, const feedRate_t fr) {
    xyze_pos_t xyze_dest = current_position;
    xyze_dest.set(dest);

    if (current_position.x == xyze_dest.x) {
        planner.buffer_line(xyze_dest, fr, PhysicalToolIndex::currently_selected());
    } else {
        feedRate_t orig_feedrate = feedrate_mm_s;
        feedrate_mm_s = fr;

        xy_float_t offset = center - current_position.xy();
        plan_arc(xyze_dest, offset, (current_position.x > xyze_dest.x), 0);

        feedrate_mm_s = orig_feedrate;
    }
}

/**
 * @brief Plan a smooth parking move from position to dock
 * @param arc_r Arc radius (mm)
 * @param pos Current position (warning: must be constant)
 * @param dock Dock position (warning: must be constant)
 * @param start_fr Starting (current) feedrate (mm/s)
 * @param end_fr Ending (parking) feedrate (mm/s)
 */
static void plan_pos2dock(const float arc_r, const xy_pos_t pos, const xy_pos_t dock,
    const float start_fr, const float end_fr) {

    // arc parameters
    xy_pos_t point;
    float arc_x;
    tangent_point(arc_r, pos, dock, arc_x, point);

    // move towards the tangent
    PrusaToolChanger::move(point.x, point.y, start_fr);
    plan_arc2(dock, { arc_x, dock.y }, start_fr);
}

/**
 * @brief Plan a smooth pickup move from dock to position
 * @param arc_r Arc radius (mm)
 * @param pos Destination position (warning: must be constant)
 * @param dock Dock position (warning: must be constant)
 * @param start_fr Starting (parking) feedrate (mm/s)
 * @param end_fr Ending (target) feedrate (mm/s)
 */
static void plan_dock2pos(const float arc_r, const xy_pos_t pos, const xy_pos_t dock,
    const float start_fr, const float end_fr) {
    // arc parameters
    xy_pos_t point;
    float arc_x;
    tangent_point(arc_r, pos, dock, arc_x, point);

    // arc to tangent point
    plan_arc2(point, { arc_x, dock.y }, end_fr);
}

} // namespace arc_move

bool PrusaToolChanger::can_move_safely(AxisHomeLevel required_level) {
    // Toolchange requires precise homing, otherwise we might not hit the docks right
    return !axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS), required_level);
}

bool PrusaToolChanger::ensure_safe_move() {
    if (!can_move_safely()) {
        // in case XY is not homed, home it first
        if (!GcodeSuite::G28_no_parser(true, true, false,
                {
                    .only_if_needed = true,
                    .z_raise = 0,
                    .precise = true, // Toolchange requires precise homing, otherwise we might not hit the docks right
                })) {
            return false;
        }
    }

    return true;
}

bool PrusaToolChanger::pick_any_tool(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool z_return) {
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        return tool_change(tool, return_type, return_position, z_lift, z_return);
    }
    return false;
}

bool PrusaToolChanger::check_emergency_stop() {
    #if HAS_CRASH_DETECTION()
    if (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) {
        return true; // Powerpanic happened, do not move and quit as soon as possible
    }
    if (quick_stopped) {
        return true; // Movements quick stoped, avoid errors that result from interrupted tool-change moves
    }
    #endif
    return false;
}

bool PrusaToolChanger::tool_change(const std::variant<PhysicalToolIndex, NoTool> new_tool, tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool z_return) {
    // WARNING: called from default(marlin) task

    // While a gcode is being unwound, every move is discarded instead of executed, so the tool
    // can neither be parked nor picked. Going through the motions anyway would leave marlin
    // believing in a tool change that never physically happened.
    if (gcode_exceptions().is_unwinding()) {
        return false;
    }

    uint8_t raw_new_tool = match(
        new_tool,
        [](PhysicalToolIndex physical_tool) { return physical_tool.to_raw(); },
        [](NoTool) { return MARLIN_NO_TOOL_PICKED; });

    // Check where we should return to
    if (return_type == tool_return_t::to_destination) {
        return_position = destination.xyz();
    }

    // if we don't know position of all axes, do not return to current position
    if (return_type == tool_return_t::to_current && !all_axes_known()) {
        return_type = tool_return_t::no_return;
    }

    // XL has no dock back-off; treat it as a plain dock
    if (return_type == tool_return_t::dock_backoff) {
        return_type = tool_return_t::no_return;
    }

    quick_stopped = false;

    // Prevent recursion
    if (block_tool_check.load()) {
        bsod("Recursion in tool_change()");
    }

    if (!is_toolchanger_enabled()) {
        if (new_tool == PhysicalToolIndex::currently_selected()) {
            return true; // Allow singletool printer to change to tool 0
        }
        toolchanger_error("Toolchanger not enabled");
    }

    Dwarf *new_dwarf = match(
        new_tool,
        [](PhysicalToolIndex tool) { return &dwarfs[tool]; },
        [](NoTool) -> Dwarf * { return nullptr; });
    if (new_dwarf && !new_dwarf->is_enabled()) {
        toolchanger_error("Toolchange to tool that is not enabled");
    }

    // Store input parameters to repeat toolchange if needed (position cleared)
    // Position is stored without tool offset, needs to be converted
    const auto store_return_position = return_position.asLogical();
    set_return_data({ new_tool, return_type, store_return_position });

    Dwarf *old_dwarf = picked_dwarf.load(); ///< Change from physically picked dwarf

    if (check_emergency_stop()) {
        return false; // Need to quit as soon as possible
    }

    planner.synchronize();

    if (check_emergency_stop()) {
        return false; // Need to quit as soon as possible
    }

    // Set block_tool_check, toolchange_in_progress and remember feedrates
    //  and reset on return from this function
    const bool levelling_active = planner.leveling_active;
    block_tool_check = true;
    #if HAS_CRASH_DETECTION()
    // Mark toolchange in progress first, to be consistent if ISR comes
    crash_s.set_toolchange_in_progress(true, levelling_active);
    #endif
    conf_restorer.sample();
    set_bed_leveling_enabled(false);
    ScopeGuard resetter = [&] {
        block_tool_check = false;
    #if HAS_CRASH_DETECTION()
        crash_s.set_toolchange_in_progress(false, levelling_active);
    #endif
        conf_restorer.restore_clear();
        set_bed_leveling_enabled(levelling_active);
        picked_update = false; // Wait for update before checking toolfall
    };

    // calculate the new tool offset difference before updating hotend_currently_applied_offset
    xyz_pos_t new_hotend_offset;
    if (new_dwarf != nullptr) {
        new_hotend_offset = hotend_offset[new_dwarf->dwarf_index()];
    } else {
        new_hotend_offset = {};
    }
    const xyz_pos_t tool_offset_diff = hotend_currently_applied_offset - new_hotend_offset; ///< Difference between offset of new and old tools
    #if HAS_NOZZLE_THERMAL_COMPENSATION()
    ///< Thermal Z term of the outgoing tool, read before active_extruder switches below
    const float thermal_z_offset_before = buddy::nozzle_thermal_compensation::current_elongation_vs_reference_mm();
    #endif

    if (new_dwarf != old_dwarf) {
        // Ensure minimal feedrate for movements
        if (feedrate_mm_s < XY_PROBE_FEEDRATE_MM_S) {
            feedrate_mm_s = XY_PROBE_FEEDRATE_MM_S;
        }

        // Raise Z before move
        float z_raise = calc_z_raise(return_type, return_position, z_lift, levelling_active);
        // if new_tool has positive offset that means Z needs to move away from print, we'll do it together with other raises to speed things up
        // (negative offset is applied later after parking current tool)
        if (tool_offset_diff.z > 0) {
            z_raise += tool_offset_diff.z;
        }
        if (z_raise > 0) {
            z_shift(z_raise);
        }

        // Home X and Y if needed
        if (!ensure_safe_move()) {
            return false;
        }

        // Park old tool
        if (old_dwarf != nullptr) {
            if (!park(*old_dwarf) || !check_skipped_step()) {
                return false;
            }
        }
    }

    // request select, wait for it to complete
    request_active_switch(new_dwarf);
    const uint8_t old_tool = active_extruder;
    active_extruder = raw_new_tool;
    update_software_endstops(X_AXIS, old_tool, raw_new_tool);
    update_software_endstops(Y_AXIS, old_tool, raw_new_tool);
    update_software_endstops(Z_AXIS, old_tool, raw_new_tool);

    hotend_currently_applied_offset = new_hotend_offset;

    // Disable print fan on old dwarf, fan on new dwarf will be enabled by marlin
    // todo: remove this when multiple fans are implemented properly
    if (old_dwarf != nullptr) {
        Fans::print(old_dwarf->tool_index()).set_pwm(0);
    }

    if (new_dwarf != old_dwarf) {
        if (new_dwarf != nullptr) {
            // Before we try to pick up new tool, check that its parked properly
            if (!new_dwarf->is_parked()) {
                log_error(PrusaToolChanger, "Trying to pick missing Dwarf #%u, triggering toolchanger recovery", new_dwarf->dwarf_index());
                toolcrash();
                return false;
            }

            if (tool_offset_diff.z < 0) {
                // positive Z diff was already applied during Z move away, now apply negative z shift (move tool down)
                z_shift(tool_offset_diff.z);
            }

            // Pick new tool
            if (!pickup(*new_dwarf) || !check_skipped_step()) {
                return false;
            }
        }

        if (check_emergency_stop()) {
            return false; // Need to quit as soon as possible
        }

        // update return_position to the new working offset
        return_position += tool_offset_diff;
    #if HAS_NOZZLE_THERMAL_COMPENSATION()
        // The two tools run at their own temperatures, so their nozzles are not the same length.
        // return_position is native, captured under the outgoing tool's term; re-express it under
        // the incoming one so the head returns to the same commanded Z rather than the same
        // carriage Z. The delta rides the return move below.
        return_position.z += buddy::nozzle_thermal_compensation::current_elongation_vs_reference_mm() - thermal_z_offset_before;
    #endif
        // Prevent a move outside physical bounds
        apply_motion_limits(return_position);

        // Move back in XY direction
        if (return_type > tool_return_t::no_return) {
            // Move back to the original (or adjusted) position
            unpark_to(return_position.xy()); // schedule a smooth XY transition to return_position
        }

        // Now move down in Z
        if (z_return) {
            set_bed_leveling_enabled(levelling_active); // Reenable MBL for this move
            if (current_position.z != return_position.z) {
                destination = current_position;
                destination.z = return_position.z;
                prepare_move_to(destination, Z_HOP_FEEDRATE_MM_S, {});
            }
        }

        if (check_emergency_stop()) {
            return false; // Need to quit as soon as possible
        }

        // Wait for moves to finish
        /// @note This synchronization makes it a bit slower, but prevents errors of powerpanic and crash
        ///   happening after this but during last moves of toolchange.
        planner.synchronize();
    }

    return true;
}

bool PrusaToolChanger::check_skipped_step() {
    #if HAS_CRASH_DETECTION()
    if (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH) {
        // Force rehome
        axes_home_level[X_AXIS] = AxisHomeLevel::not_homed;
        axes_home_level[Y_AXIS] = AxisHomeLevel::not_homed;
        if (ensure_safe_move()) {
            crash_s.set_state(Crash_s::REPEAT_WAIT); // Has to go through WAIT to PRINTING
            crash_s.set_state(Crash_s::PRINTING);
        } else {
            return false;
        }
    }
    #endif

    return true;
}

    #if HAS_TOOL_CRASH_RECOVERY()
void PrusaToolChanger::crash_deselect_tool() {
    if (PhysicalToolIndex::currently_selected_opt().has_value()) {
        prusa_toolchanger.request_active_switch(nullptr); // Deselect dwarf
        const uint8_t old_tool_index = active_extruder;
        active_extruder = PrusaToolChanger::MARLIN_NO_TOOL_PICKED; // Mark no tool for Marlin
        update_software_endstops(X_AXIS, old_tool_index, PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
        update_software_endstops(Y_AXIS, old_tool_index, PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
        update_software_endstops(Z_AXIS, old_tool_index, PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    }
}
    #endif

void PrusaToolChanger::toolcheck_enable() {
    const auto selected_tool = PhysicalToolIndex::currently_selected();

    // Revert selected to active tool
    for (auto tool : PhysicalToolIndex::all()) {
        /// TODO do not access puppyModbus outside of puppy task
        /// BFW-8185
        getTool(tool).set_selected(buddy::puppies::puppyModbus, tool.is_enabled() && stdext::holds_value(selected_tool, tool));
    }

    if (!block_tool_check.exchange(false)) { // Test if was blocked
        bsod("Toolchange loop() enabled but not blocked");
    }
}

void PrusaToolChanger::toolcrash() {
    // A gcode cancelled mid-toolchange has its park/pick moves discarded, so the dock sensors
    // rightly report a tool that never moved. That is the cancellation working, not a crash, and
    // it must not start a crash recovery that would fight the unwinding either.
    if (gcode_exceptions().is_unwinding()) {
        return;
    }

    #if HAS_CRASH_DETECTION()
    if (crash_s.is_active() && (crash_s.get_state() == Crash_s::PRINTING)) {
        crash_s.set_state(Crash_s::TRIGGERED_TOOLCRASH); // Trigger recovery process
        return;
    }

    if (((crash_s.get_state() == Crash_s::RECOVERY) && crash_s.is_toolchange_event()) // Already recovering, cannot disrupt crash_s, it will replay Tx gcode
        || (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) // Power panic, end quickly and don't do anything
        || (crash_s.get_state() == Crash_s::TRIGGERED_ISR) // ISR crash happened, it will replay Tx gcode
        || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLFALL) // Toolcrash is already in progress
        || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH)
        || (crash_s.get_state() == Crash_s::REPEAT_WAIT)
        || (crash_s.get_state() == Crash_s::SELFTEST)
        || quick_stopped) {
        return; // Ignore
    }
    #endif

    // Can happen if toolchange is a part of replay, would need a bigger change in crash_recovery.cpp
    toolchanger_error("Tool crashed");
}

void PrusaToolChanger::toolfall() {
    #if HAS_CRASH_DETECTION()
    if (crash_s.is_active() && (crash_s.get_state() == Crash_s::PRINTING)) {
        crash_s.set_state(Crash_s::TRIGGERED_TOOLFALL);
        return;
    }

    if ((crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) // Power panic, end quickly and don't do anything
        || (crash_s.get_state() == Crash_s::IDLE)) { // Print ended
        return;
    }
    #endif

    // Can be called when starting the print, not a big problem
    // Can happen if tool falls of during recovery, homing or reheating, would need a bigger change in crash_recovery.cpp
    toolchanger_error("Tool fell off");
}

bool PrusaToolChanger::purge_tool(PhysicalToolIndex tool) {
    auto &dwarf = dwarfs[tool];

    if (thermalManager.tooColdToExtrude(tool)) {
        // hotend is cold, skip purge because it can't do anything
        return true;
    }

    // fan to 100% for better sopel
    // use fanctl interface directly, without modifing marlin's value. This will prevent restoring wrong fan value on power panic or failed toolchange.
    // !!! Note: This does not work if you're purging the currently selected tool - see BFW-6365
    const auto prev_pwm = Fans::print(tool).get_pwm();
    Fans::print(tool).set_pwm(255);

    // go to purge location
    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/true);
    move(info.dock_x - 9.9f, PURGE_Y_POSITION, feedrate_mm_s);

    planner.synchronize();

    // wait a while for fan to spool up
    (void)wait([]() { return false; }, 2000);

    // extrude some filament, park&pick it again, to wipe it
    auto orig_e_pos = current_position.e;

    // current filament type
    const auto filament = FilamentType::for_current_tool_heuristic();

    // extrude
    mapi::extruder_move(ADVANCED_PAUSE_PURGE_LENGTH, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::advanced_pause_purge, filament));

    #if HAS_AUTO_RETRACT()
    // Only retract if HAS_AUTO_RETRACT — otherwise the retract would
    // leave a permanent gap (the planner has no record of the retract due to
    // the sync_e_position_to below).
    mapi::extruder_move(-STANDARD_RETRACT_LENGTH, buddy::standard_feedrates::extruder(buddy::standard_feedrates::Extruder::retract, filament));

    // The retraction must be deretracted though, otherwise we would have a hole in the print
    buddy::auto_retract().set_retracted_distance(tool, STANDARD_RETRACT_LENGTH);
    #endif

    planner.synchronize();

    // reset position back, like purge never happened
    sync_e_position_to(orig_e_pos);

    // wait a while to let the sopel cool down
    (void)wait([]() { return false; }, 5000);

    // restore fan speed
    Fans::print(tool).set_pwm(prev_pwm);

    // park() and pickup() restore the planner jerk and acceleration from conf_restorer
    // so it must be sampled before calling them, and restored after
    conf_restorer.sample();
    ScopeGuard conf_resetter = [&] { conf_restorer.restore_clear(); };

    if (!park(dwarf)) {
        return false;
    }
    if (!pickup(dwarf)) {
        return false;
    }

    return true;
}

void PrusaToolChanger::loop(bool printing, bool paused) {
    // WARNING: called from default(marlin) task

    if (block_tool_check.load() // This function can be blocked
        || !is_toolchanger_enabled()) { // Ignore on singletool
        return;
    }

    // Wait until picked_dwarf is refreshed
    if (picked_update.load()) {
        const Dwarf *picked = picked_dwarf.load();
        const Dwarf *active = get_marlin_picked_tool();
        picked_update = false;

        // Automatically change tool
        if (force_toolchange_gcode.load() // Force toolchange after reset to force all marlin tool variables
            || ((picked != active) // When user parked or picked manually
                && (printing == false) && (paused == false) // Only if not printing and not in pause
                && (queue.has_commands_queued() == false) && (planner.processing() == false))) { // And nothing is in queue
            force_toolchange_gcode = false;

            // Update tool through marlin
            marlin_server::enqueue_gcode_printf("T%d S1 M0", detect_tool_nr());
        }

        // Check that all tools are where they should be
        if (printing // Only while printing
    #if HAS_CRASH_DETECTION()
            && (crash_s.get_state() == Crash_s::PRINTING) // Do not check during crash recovery
    #endif
            && !Pause::Instance().get_mode().has_value()) { // Do not check during filament change
            bool all_good = true;
            if (picked != active) {
                tool_check_fails++; // Tool switched
                all_good = false;
            }
            for (Dwarf &dwarf : dwarfs) {
                if (dwarf.is_enabled() && (dwarf.is_picked() == false) && (dwarf.is_parked() == false)) {
                    tool_check_fails++; // Tool fell off
                    all_good = false;
                }
            }
            if (all_good) {
                tool_check_fails = 0;
            } else if (tool_check_fails >= TOOL_CHECK_FAILS_LIMIT) {
                toolfall();
                tool_check_fails = 0; // Clear fail counter also on toolfall
            }
        }
    }

    // Update the currently applied offset when idling (so that a manual swap is reflected), but
    // _not_ during print where toolchange() is in charge to do the heavy lifting
    if (!printing) {
        const auto physical_tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
        if (physical_tool.has_value()) {
            hotend_currently_applied_offset = hotend_offset[*physical_tool];
        }
    }
}

void PrusaToolChanger::move(const float x, const float y, const feedRate_t feedrate) {
    current_position.x = x;
    current_position.y = y;
    line_to_current_position(feedrate);
}

const xy_float_t PrusaToolChanger::get_tool_dock_position(PhysicalToolIndex tool, bool check_calibrated) {
    const auto info = PrusaToolChangerUtils::get_tool_info(tool, check_calibrated);
    return xy_float_t { info.dock_x, SAFE_Y_WITH_TOOL };
}

xy_pos_t PrusaToolChanger::tool_park_position(PhysicalToolIndex tool) {
    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/false);

    // This position should 1:1 match the initial position for tool parking
    return xy_pos_t { .x = info.dock_x + PARK_X_OFFSET_1, .y = SAFE_Y_WITH_TOOL };
}

bool PrusaToolChanger::park(Dwarf &dwarf) {
    auto dwarf_parked = [&dwarf]() {
        if (!dwarf.refresh_park_pick_status()) {
            return false;
        }
        return dwarf.is_parked();
    };

    auto dwarf_not_picked = [&dwarf]() {
        if (!dwarf.refresh_park_pick_status()) {
            return false;
        }
        return !dwarf.is_picked();
    };

    const PrusaToolInfo &info = get_tool_info(dwarf.tool_index(), /*check_calibrated=*/true);

    // safe target dock position
    const xy_pos_t target_pos = tool_park_position(dwarf.tool_index());

    // reduce maximum parking speed to improve reliability during constant toolchanging
    float target_fr = limit_stealth_feedrate(PARKING_FINAL_MAX_SPEED);
    float tangent_fr = limit_stealth_feedrate(TRAVEL_MOVE_MM_S);

    // Arc will not be limited by jerk
    planner.set_max_jerk(AxisEnum::X_AXIS, arc_move::arc_tg_jerk);
    planner.set_max_jerk(AxisEnum::Y_AXIS, arc_move::arc_tg_jerk);

    // attempt to plan a smooth arc move
    float arc_r = arc_move::arc_radius(current_position.xy(), target_pos);
    if (arc_r >= arc_move::arc_min_radius) {
        if (arc_r < arc_move::arc_max_radius) {
            // Arc is smaller than typical, need to slow down
            static_assert(arc_move::arc_max_radius != arc_move::arc_min_radius, "These cannot be equal");
            float rel_size = (arc_r - arc_move::arc_min_radius) / (arc_move::arc_max_radius - arc_move::arc_min_radius);
            tangent_fr = fminf(tangent_fr, PARKING_FINAL_MAX_SPEED + rel_size * (TRAVEL_MOVE_MM_S - PARKING_FINAL_MAX_SPEED));
        }
        arc_move::plan_pos2dock(arc_r, current_position.xy(), target_pos,
            tangent_fr, target_fr);
    }

    // go in front of the tool dock
    move(target_pos.x, target_pos.y, target_fr);
    move(target_pos.x, info.dock_y, target_fr);
    planner.synchronize(); // this creates a pause which allow the resonance in the tool to be damped before insertion of the tool in the dock
    conf_restorer.restore_jerk();

    { // Set motor current and stall sensitivity to parking and remember old value
        StepperConfigGuard sc;

        move(info.dock_x + PARK_X_OFFSET_2, info.dock_y, SLOW_MOVE_MM_S);
        {
            auto s = planner.user_settings;
            s.travel_acceleration = SLOW_ACCELERATION_MM_S2;
            planner.apply_settings(s);
        }
        move(info.dock_x + PARK_X_OFFSET_3, info.dock_y, SLOW_MOVE_MM_S);
        planner.synchronize();
        conf_restorer.restore_acceleration(); // back to high acceleration
    }

    move(info.dock_x, info.dock_y, SLOW_MOVE_MM_S);
    planner.synchronize();

    // Wait until dwarf is registering as parked
    if (!wait(dwarf_parked, WAIT_TIME_TOOL_PARKED_PICKED)) {
        log_warning(PrusaToolChanger, "Dwarf #%u not parked, trying to wiggle it in", dwarf.dwarf_index());

        move(info.dock_x - DOCK_WIGGLE_OFFSET, info.dock_y, SLOW_MOVE_MM_S); // wiggle left
        move(info.dock_x, info.dock_y, SLOW_MOVE_MM_S); // wiggle back
        planner.synchronize();

        if (!wait(dwarf_parked, WAIT_TIME_TOOL_PARKED_PICKED)) {
            log_error(PrusaToolChanger, "Dwarf #%u not parked, triggering toolchanger recovery", dwarf.dwarf_index());
            toolcrash();
            return false;
        }
    }

    move(info.dock_x, SAFE_Y_WITHOUT_TOOL, feedrate_mm_s); // release tool

    // Wait until dwarf is registering as not picked
    if (!wait(dwarf_not_picked, WAIT_TIME_TOOL_PARKED_PICKED)) {
        log_warning(PrusaToolChanger, "Dwarf #%u still picked after parking, waiting for pull to finish", dwarf.dwarf_index());

        // Can happen if parking in really low speed and acceleration
        planner.synchronize(); // Just wait for the pull move to finish and check again

        if (!wait(dwarf_not_picked, WAIT_TIME_TOOL_PARKED_PICKED)) {
            log_error(PrusaToolChanger, "Dwarf #%u still picked after parking, triggering toolchanger recovery", dwarf.dwarf_index());
            toolcrash();
            return false;
        }
    }
    log_info(PrusaToolChanger, "Dwarf #%u parked successfully", dwarf.dwarf_index());
    return true;
}

bool PrusaToolChanger::align_locks() {
    if (picked_dwarf.load() != nullptr) {
        return true; // It would catapult picked dwarf
    }

    #if HAS_CRASH_DETECTION()
    // Disable crash detection, would result in bsod, this is followed by homing anyway
    Crash_Temporary_Deactivate ctd;
    #endif

    const float travel_feedrate = limit_stealth_feedrate(TRAVEL_MOVE_MM_S);

    // Move to safe position before homing move
    if (can_move_safely(AxisHomeLevel::imprecise)) {
        if (current_position.y > SAFE_Y_WITHOUT_TOOL) {
            move(current_position.x, SAFE_Y_WITHOUT_TOOL, SLOW_MOVE_MM_S); // To safe Y
            planner.synchronize();
        }

        // Go to the front right corner quickly, so homing is not too long
        move(X_MAX_POS, 0, travel_feedrate);
    } else {
        // Move a bit back in case the carriage is near tool
        if (do_homing_move(Y_AXIS, SAFE_Y_WITHOUT_TOOL - DOCK_DEFAULT_Y_MM)) {
            // If we hit something, it means we're at the front of the printer (Y_MIN)
            // Back of a bit, otherwise we'll be sliding on the front edge, falsely triggering Y endstops during the X move
            const bool backoff_move_hit = do_homing_move(Y_AXIS, 10);

            if (backoff_move_hit) {
                // If we've hit something in both directions, something's wrong
                toolchanger_error("Y axis stuck");
            }

            // Check for the correct backoff direction
            static_assert(SAFE_Y_WITHOUT_TOOL - DOCK_DEFAULT_Y_MM < -10);
        }
    }
    planner.synchronize();

    // Bump right edge
    if (!do_homing_move(X_AXIS, 1.5f * max_length(X_AXIS), 0)) {
        axes_home_level[X_AXIS] = AxisHomeLevel::not_homed; // Needs homing
        return false; // Failed to bump right edge
    }

    // Return to left edge before homing
    move(0, current_position.y, travel_feedrate);
    planner.synchronize();

    return true;
}

bool PrusaToolChanger::pickup(Dwarf &dwarf) {
    auto dwarf_picked = [&dwarf]() {
        if (!dwarf.refresh_park_pick_status()) {
            return false;
        }
        return dwarf.is_picked();
    };

    auto dwarf_not_parked = [&dwarf]() {
        if (!dwarf.refresh_park_pick_status()) {
            return false;
        }
        return !dwarf.is_parked();
    };

    const PrusaToolInfo &info = get_tool_info(dwarf.tool_index(), /*check_calibrated=*/true);

    const auto limited_feedrate = limit_stealth_feedrate(feedrate_mm_s);

    move(info.dock_x, SAFE_Y_WITHOUT_TOOL, limited_feedrate); // go in front of the tool
    move(info.dock_x, info.dock_y + PICK_Y_OFFSET, limited_feedrate); // pre-insert fast the tool

    {
        auto s = planner.user_settings;
        s.travel_acceleration = SLOW_ACCELERATION_MM_S2;
        planner.apply_settings(s);
    }

    move(info.dock_x, info.dock_y, SLOW_MOVE_MM_S); // insert slowly the last mm to allow part fitting + soft touch between TCM and tool thanks to the gentle deceleration
    planner.synchronize();

    // Wait until dwarf is registering as picked
    if (!wait(dwarf_picked, WAIT_TIME_TOOL_PARKED_PICKED)) {
        log_warning(PrusaToolChanger, "Dwarf #%u not picked, trying to wiggle it in", dwarf.dwarf_index());

        move(info.dock_x, info.dock_y + DOCK_WIGGLE_OFFSET, SLOW_MOVE_MM_S); // wiggle pull
        move(info.dock_x, info.dock_y, SLOW_MOVE_MM_S); // wiggle back
        planner.synchronize();

        if (!wait(dwarf_picked, WAIT_TIME_TOOL_PARKED_PICKED)) {
            log_error(PrusaToolChanger, "Dwarf #%u not picked, triggering toolchanger recovery", dwarf.dwarf_index());
            toolcrash();
            return false;
        }
    }

    { // Set motor current and stall sensitivity to parking and remember old value
        StepperConfigGuard sc;

        move(info.dock_x + PICK_X_OFFSET_1, info.dock_y, SLOW_MOVE_MM_S); // accelerate gently to low speed to gently place the tool against the TCM
        conf_restorer.restore_acceleration(); // back to high acceleration
        move(info.dock_x + PICK_X_OFFSET_2, info.dock_y, SLOW_MOVE_MM_S); // this line is just to allow a gentle acceleration and a quick deceleration
        move(info.dock_x + PICK_X_OFFSET_3, info.dock_y, SLOW_MOVE_MM_S);
        planner.synchronize();
    }

    // Wait until dwarf is registering as not parked
    if (!wait(dwarf_not_parked, WAIT_TIME_TOOL_PARKED_PICKED)) {
        log_error(PrusaToolChanger, "Dwarf #%u still parked after picking, triggering toolchanger recovery", dwarf.dwarf_index());
        toolcrash();
        return false;
    }

    move(info.dock_x + PICK_X_OFFSET_3, SAFE_Y_WITH_TOOL, limited_feedrate); // tool extracted

    log_info(PrusaToolChanger, "Dwarf #%u picked successfully", dwarf.dwarf_index());
    Odometer_s::instance().add_toolpick(PhysicalToolIndex::from_raw(dwarf.dwarf_index())); // Count picks
    return true;
}

void PrusaToolChanger::unpark_to(const xy_pos_t &destination) {
    // Limit feedrate during the arc
    float arc_fr = limit_stealth_feedrate(TRAVEL_MOVE_MM_S);

    // Arc will not be limited by jerk
    planner.set_max_jerk(AxisEnum::X_AXIS, arc_move::arc_tg_jerk);
    planner.set_max_jerk(AxisEnum::Y_AXIS, arc_move::arc_tg_jerk);

    // attempt to plan a smooth arc move
    float arc_r = arc_move::arc_radius(destination, current_position.xy());
    if (arc_r >= arc_move::arc_min_radius) {
        if (arc_r < arc_move::arc_max_radius) {
            // Arc is smaller than typical, need to slow down
            static_assert(arc_move::arc_max_radius != arc_move::arc_min_radius, "These cannot be equal");
            float rel_size = (arc_r - arc_move::arc_min_radius) / (arc_move::arc_max_radius - arc_move::arc_min_radius);
            arc_fr = fminf(arc_fr, PARKING_FINAL_MAX_SPEED + rel_size * (TRAVEL_MOVE_MM_S - PARKING_FINAL_MAX_SPEED));
        }

        arc_move::plan_dock2pos(arc_r, destination, current_position.xy(), arc_fr, arc_fr);
    }

    // move to the destination
    move(destination.x, destination.y, TRAVEL_MOVE_MM_S);

    conf_restorer.restore_jerk();
}

#endif
