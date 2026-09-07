#include "toolchanger.h"
#include "module/planner.h"
#include "module/tool_change.h"
#include <tool_index.hpp>
#include <module/endstops.h>

#include <option/has_crash_detection.h>
#include <option/has_power_panic.h>
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#include <raii/scope_guard.hpp>

#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/stepper/trinamic.h"
#include "Marlin/src/module/motion.h"
#include "Marlin/src/feature/bedlevel/bedlevel.h"
#include "Marlin/src/gcode/gcode.h"
#include <logging/log.hpp>
#include "timing.h"
#include "fanctl.hpp"
#include <marlin_server.hpp>
#include <marlin_vars.hpp>
#include <fsm/nozzle_mismatch_phases.hpp>
#include <cmath_ext.h>
#include <odometer.hpp>
#include <pause_stubbed.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>
#include "module/temperature.h" // for fan control
#include <tool/hotend/hotend/indx_hotend.hpp>
#include <raii/auto_restore.hpp>
#include <mapi/parking.hpp>
#include <feature/indx_tool_lock_hack/indx_tool_lock_hack.hpp>
#include <bsod/bsod.h>

#if HAS_CRASH_DETECTION()
    #include "../../feature/prusa/crash_recovery.hpp"
#endif

#if HAS_POWER_PANIC()
    #include <power_panic.hpp>
    #include <tasks.hpp>
#endif

#if DISABLED(ARC_SUPPORT)
    #error "toolchanger requires ARC_SUPPORT"
#endif

#include <option/has_nozzle_thermal_compensation.h>
// The nozzle thermal compensation re-expresses the toolchange return position under the incoming
// tool's nozzle length. That hook exists in the XL toolchanger only, so enabling the option here
// would compile but return the head to the previous tool's compensated height.
static_assert(!HAS_NOZZLE_THERMAL_COMPENSATION(), "no nozzle thermal compensation hook in the INDX toolchanger");

LOG_COMPONENT_REF(PrusaToolChanger);

PrusaToolChanger prusa_toolchanger;

// internal helpers for arc planning
void plan_arc(const xyze_pos_t &cart, const xy_float_t &offset, const bool clockwise, const uint8_t circles);

namespace arc_move {

// generated arc parameters
constexpr float arc_max_radius = 75.f; // mm
constexpr float arc_min_radius = 10.f; // mm
constexpr float arc_tg_jerk = 20.f; // mm/s
constexpr bool arc_backtravel_allow = true;
constexpr float arc_backtravel_max = 2.f; // 1/ratio

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
                    .precise = true, // Toolchange requires precise homing, otherwise we might not hit the docks right
                })) {
            return false;
        }
    }

    return true;
}

bool PrusaToolChanger::pick_any_tool(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool z_return) {
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        if (tool_change(tool, return_type, return_position, z_lift, z_return)) {
            return true;
        }
    }
    return false;
}

bool PrusaToolChanger::tool_change(const std::variant<PhysicalToolIndex, NoTool> new_tool, tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool z_return) {
    // WARNING: called from default(marlin) task

    quick_stopped = false;

    // Prevent recursion
    if (block_tool_check.load()) {
        bsod("Recursion in tool_change()");
    }
    block_tool_check.store(true);
    // Declared first so it runs last — after the trailing planner.synchronize() — keeping
    // phase_ set while toolchange moves are still in the queue.
    ScopeGuard restore_block_tool_check([&]() {
        block_tool_check.store(false);
        phase_.store(ToolchangePhase::none, std::memory_order_release);
#if HAS_CRASH_DETECTION()
        crash_s.set_toolchange_in_progress(false, false);
#endif
    });

    // Check where we should return to
    if (return_type == tool_return_t::to_destination) {
        return_position = destination.xyz();
    }

    // if we don't know position of all axes, do not return to current position
    if (return_type == tool_return_t::to_current && !all_axes_known()) {
        return_type = tool_return_t::no_return;
    }

    // Publish toolchange-return data and the before_lock phase. Order matters (atomics synchronize).
    set_return_data({ new_tool, return_type, return_position.asLogical() });
    phase_.store(ToolchangePhase::before_lock, std::memory_order_release);
#if HAS_CRASH_DETECTION()
    crash_s.set_toolchange_in_progress(true, planner.leveling_active);
#endif

    const auto old_tool = PhysicalToolIndex::currently_selected();

    if (std::holds_alternative<PhysicalToolIndex>(new_tool) && !std::get<PhysicalToolIndex>(new_tool).is_enabled()) {
        toolchanger_error("Toolchange to tool that is not enabled");
    }

    planner.synchronize();

    const bool levelling_active = planner.leveling_active;
    set_bed_leveling_enabled(false);
    ScopeGuard restore_leveling([&]() {
        set_bed_leveling_enabled(levelling_active);
    });

    // calculate the new tool offset difference before updating hotend_currently_applied_offset
    xyz_pos_t new_hotend_offset;

    // Set up new hotend offset
    match(
        new_tool,
        [&](PhysicalToolIndex physical_tool) { new_hotend_offset = hotend_offset[physical_tool]; },
        [&](NoTool) { new_hotend_offset = {}; });
    const xyz_pos_t tool_offset_diff = hotend_currently_applied_offset - new_hotend_offset; ///< Difference between offset of new and old tools

    if (new_tool != old_tool) {
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
        if (std::holds_alternative<PhysicalToolIndex>(old_tool)) {
            if (!park(std::get<PhysicalToolIndex>(old_tool))) {
                return false;
            }
        }
    }

    update_software_endstops(X_AXIS);
    update_software_endstops(Y_AXIS);
    update_software_endstops(Z_AXIS);

    if (new_tool != old_tool) {
        if (std::holds_alternative<PhysicalToolIndex>(new_tool)) {
            if (tool_offset_diff.z < 0) {
                // positive Z diff was already applied during Z move away, now apply negative z shift (move tool down)
                z_shift(tool_offset_diff.z);
            }

            // Pick new tool
            if (!pickup(std::get<PhysicalToolIndex>(new_tool))) {
                return false;
            }
        }

        final_tool_change_moves({ .return_position = return_position, .return_type = return_type, .levelling_active = levelling_active, .z_return = z_return, .tool_offset_diff = tool_offset_diff });
    }

    return true;
}

/// Lower the bed (print-end height) so the user can pick up a failed print and reach the docks.
static void lower_bed_for_pickup() {
    mapi::park({ .z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE, .absolute = Z_NOZZLE_PARK_POINT_MIN } });
}

void PrusaToolChanger::check_nozzle_presence_vs_eeprom() {
#if HAS_POWER_PANIC()
    // Stored panic data + autostart not yet done = resume decision pending.
    // EEPROM is intentionally stale during prints, so this check would false-fire here.
    if (!TaskDeps::check(TaskDeps::Dependency::autostart_done)
        && power_panic::state_stored()) {
        return;
    }
#endif

    const auto maybe_nozzle = buddy::puppies::indx.get_nozzle_present();
    if (!maybe_nozzle.has_value()) {
        return; // No debounced value yet (boot or after invalidation)
    }

    // Don't open the nozzle mismatch dialog if another FSM (e.g. calibration) is already active
    const bool any_fsm_active = marlin_vars().peek_fsm_states(
        [](const auto &states) { return states.get_top().has_value(); });
    if (any_fsm_active) {
        return;
    }

    const bool nozzle_present = maybe_nozzle.value();
    const bool eeprom_valid = config_store().indx_last_picked_tool_valid.get();
    const auto eeprom_tool = config_store().get_indx_last_picked_tool();
    const bool eeprom_says_tool = eeprom_valid && std::holds_alternative<PhysicalToolIndex>(eeprom_tool);
    const bool eeprom_says_no_tool = !eeprom_valid || std::holds_alternative<NoTool>(eeprom_tool);

    if (eeprom_says_no_tool && nozzle_present) {
        // EEPROM says no tool, but a nozzle is detected — ask user which dock it belongs to
        log_error(PrusaToolChanger, "Nozzle detected but EEPROM says no tool");
        manual_tool_park();
    } else if (eeprom_says_tool && !nozzle_present) {
        // EEPROM says tool is picked, but no nozzle is detected — correct to no tool
        const auto tool_index = std::get<PhysicalToolIndex>(eeprom_tool);
        log_warning(PrusaToolChanger, "EEPROM says tool #%u but no nozzle detected, correcting to no tool", tool_index.to_raw());
        set_active_extruder(NoTool {}); // Should be already NoTool
        persist_last_picked_tool(NoTool {}, /*override_always=*/true);

        // Position is likely off (skipped steps during failed pickup) — force rehome before next toolchange
        invalidate_xy_homing();

        // Inform user about lost tool, then rehome
        block_tool_check = true;
        ScopeGuard guard = [this] { block_tool_check = false; };
        marlin_server::FSM_Holder fsm(PhaseNozzleMismatch::tool_lost);

        // Lower the bed so the user can pick up the failed print while the dialog is shown.
        lower_bed_for_pickup();

        marlin_server::wait_for_response(PhaseNozzleMismatch::tool_lost);

        marlin_server::fsm_change(PhaseNozzleMismatch::homing);
        (void)ensure_safe_move();
    }
}

void PrusaToolChanger::check_nozzle_presence_during_print() {
    if (!marlin_server::is_printing()) {
        return; // not actively printing (paused/pausing/finishing/idle)
    }

    const uint32_t now = ticks_ms();
    if (ticks_diff(now, last_print_nozzle_check_ms) < int32_t(PRINT_NOZZLE_CHECK_PERIOD_MS)) {
        return;
    }
    last_print_nozzle_check_ms = now;

    const auto maybe_nozzle = buddy::puppies::indx.get_nozzle_present();
    if (!maybe_nozzle.has_value()) {
        return; // no debounced sample yet
    }

    const bool nozzle_present = *maybe_nozzle;
    const auto selected = PhysicalToolIndex::currently_selected();

    if (!std::holds_alternative<PhysicalToolIndex>(selected)) {
        if (nozzle_present) {
            // Odd state but not the failure mode we care about (tool falling off).
            log_warning(PrusaToolChanger, "In-print nozzle detected with no tool selected (ignored)");
        }
        return;
    }

    if (nozzle_present) {
        return; // expected tool, nozzle there — all good
    }

    // Tool selected but nozzle missing — likely tool fell off mid-print.
    const auto tool_index = std::get<PhysicalToolIndex>(selected);
    log_error(PrusaToolChanger, "In-print nozzle missing for tool #%u", tool_index.to_raw());

#if HAS_TOOL_CRASH_RECOVERY()
    if (crash_s.is_active()) {
        if (crash_s.get_state() == Crash_s::PRINTING) {
            log_info(PrusaToolChanger, "triggering crash recovery");
            crash_s.set_state(Crash_s::TRIGGERED_TOOLFALL);
        } else {
            // crash_s is probably in some crashed or recovering state
            // just wait until the printer finishes whatever it is doing
            // it will be eventually triggered again in PRINTING state and handled
        }
        return;
    }
#endif

    log_error(PrusaToolChanger, "crash recovery is off"); // not active (serial printing?) or disabled completely
    toolchanger_error("Tool lost");
}

#if HAS_TOOL_CRASH_RECOVERY()
void PrusaToolChanger::crash_deselect_tool() {
    set_active_extruder(NoTool {});
}
#endif

void PrusaToolChanger::invalidate_xy_homing() {
    axes_home_level[X_AXIS] = AxisHomeLevel::not_homed;
    axes_home_level[Y_AXIS] = AxisHomeLevel::not_homed;
}

void PrusaToolChanger::ensure_tool_homeable(PhysicalToolIndex tool) {
    // Tool 8 (index 7) docks at the far right edge of X travel.
    // Move away so the head doesn't collide during X homing.
    if (tool.to_raw() == 7) {
        move(current_position.x - 40.0f, current_position.y, TRAVEL_MOVE_MM_S);
        planner.synchronize();
    }
}

void PrusaToolChanger::loop(bool printing, bool paused) {
    // WARNING: called from default(marlin) task

    if (block_tool_check.load()) { // This function can be blocked
        return;
    }

    if (!nozzle_check_disabled) {
        if (printing && !paused) {
            // EEPROM is invalidated during prints, so use the in-RAM active tool instead.
            check_nozzle_presence_during_print();
        } else {
            check_nozzle_presence_vs_eeprom();
        }
    }

    // set_active_extruder() owns the applied offset on every tool change. While idle, also re-derive it
    // here so a table edit to the active tool's offset (e.g. M218) takes effect without a tool change.
    // Skipped during print, where the applied offset must change only in lockstep with toolchange moves.
    if (!printing) {
        hotend_currently_applied_offset = match(
            PhysicalToolIndex::currently_selected(),
            [](PhysicalToolIndex tool) -> xyz_pos_t { return hotend_offset[tool]; },
            [](NoTool) -> xyz_pos_t { return { 0, 0, 0 }; });
    }
}

void PrusaToolChanger::move(const float x, const float y, const feedRate_t feedrate) {
    current_position.x = x;
    current_position.y = y;
    line_to_current_position(feedrate);
}

const xy_float_t PrusaToolChanger::get_tool_dock_position(PhysicalToolIndex tool, bool check_calibrated) {
    const auto info = PrusaToolChangerUtils::get_tool_info(tool, check_calibrated);
    return xy_float_t { info.dock_x, info.dock_y + DOCK_SAFE_Y_OFFSET };
}

/// Relative E-axis move with service hints (bypasses cold extrusion and filament tracking)
static void e_move(float distance, float feedrate) {
    xyze_pos_t target = current_position;
    target.e += distance;
    const PrepareMoveHints hints = {
        .scale_feedrate = false,
        .do_segment = false,
        .apply_motion_limits = false,
        .move = { .extrusion_safety_checks = false, .is_service_extruder_move = true },
    };

    prepare_move_to(target, feedrate, hints);
}

/// RAII guard that saves and restores E position and E motor current.
class EMotorGuard {
public:
    EMotorGuard()
        : orig_e_pos_(current_position.e)
        , orig_e_current_(stepperE0.rms_current()) {}

    ~EMotorGuard() {
        stepperE0.rms_current(orig_e_current_);
        current_position.e = orig_e_pos_;
        sync_plan_position_e(E0_AXIS);
    }

    EMotorGuard(const EMotorGuard &) = delete;
    EMotorGuard &operator=(const EMotorGuard &) = delete;

private:
    float orig_e_pos_;
    uint16_t orig_e_current_;
};

bool PrusaToolChanger::ensure_head_open(std::variant<PhysicalToolIndex, NoTool> tool) {
    if (head_open) {
        return true;
    } else if (!ensure_safe_move()) {
        return false;
    }
    const auto physical_tool = std::holds_alternative<PhysicalToolIndex>(tool)
        ? std::get<PhysicalToolIndex>(tool)
        : PhysicalToolIndex::from_raw(0);
    open_head(physical_tool);
    return true;
}

bool PrusaToolChanger::manual_tool_park(std::optional<PhysicalToolIndex> tool) {
    // Bump → set_active → persist → park sequence shared by both paths.
    // On dock-occupied this surfaces the dock_not_empty FSM phase so both the
    // interactive and the direct caller get visual feedback.
    const auto dock_tool = [this](PhysicalToolIndex t) -> bool {
        if (!bump_to_dock(t)) {
            log_warning(PrusaToolChanger, "Manual park: bump to dock %u failed", t.to_raw());
            marlin_server::fsm_change(PhaseNozzleMismatch::dock_not_empty);
            marlin_server::wait_for_response(PhaseNozzleMismatch::dock_not_empty);
            return false;
        }
        log_info(PrusaToolChanger, "Bump to dock %u succeeded, parking tool", t.to_raw());
        set_active_extruder(t);
        persist_last_picked_tool(t);
        if (!park(t)) {
            log_warning(PrusaToolChanger, "Manual park of tool #%u failed", t.to_raw());
            return false;
        }
        return true;
    };

    // Prevent re-entry: wait_for_response() runs the idle loop, which calls
    // loop() -> check_nozzle_presence_vs_eeprom() again
    block_tool_check = true;
    ScopeGuard guard = [this] { block_tool_check = false; };

    // Direct path — caller already knows which dock to use. Still show the
    // parking FSM so the user gets visual feedback during the procedure.
    if (tool) {
        marlin_server::FSM_Holder fsm(PhaseNozzleMismatch::parking);
        return dock_tool(*tool);
    }

    // Open on the buttonless homing screen so the prompt isn't shown while the head moves.
    marlin_server::FSM_Holder fsm(PhaseNozzleMismatch::homing);

    // Lower the bed so the user can pick up a failed print and reach the docks.
    lower_bed_for_pickup();

    // Interactive path — drive the nozzle-mismatch FSM so the user picks a dock
    marlin_server::fsm_change(PhaseNozzleMismatch::prompt);
    [[maybe_unused]] const auto prompt_response = marlin_server::wait_for_response(PhaseNozzleMismatch::prompt);

    while (true) {
        marlin_server::fsm_change(PhaseNozzleMismatch::dock_selection);
        const auto response = marlin_server::wait_for_response_variant(PhaseNozzleMismatch::dock_selection);
        const auto *raw_dock = response.value_maybe<uint8_t>();
        if (!raw_dock) {
            // User aborted dock selection (e.g. Back) — exit cleanly
            log_info(PrusaToolChanger, "Manual park: dock selection aborted");
            return false;
        }
        const auto tool_index = PhysicalToolIndex::from_raw(*raw_dock);

        log_info(PrusaToolChanger, "User selected dock %u for unknown nozzle", *raw_dock);

        marlin_server::fsm_change(PhaseNozzleMismatch::parking);
        if (dock_tool(tool_index)) {
            return true; // FSM_Holder destructor closes the dialog
        }
        // dock_tool failed; on dock-occupied it already showed dock_not_empty
        // and waited. Loop back to dock_selection so the user can try another.
    }
}

void PrusaToolChanger::wiggle_and_partial_unlock() {
    stepperE0.rms_current(E_WIGGLE_CURRENT_MA);

    // Wiggle E to align unlock teeth
    e_move(-E_WIGGLE_DISTANCE, E_WIGGLE_FEEDRATE);
    e_move(+E_WIGGLE_DISTANCE, E_WIGGLE_FEEDRATE);
    planner.synchronize();

    // Increase E current for actual unlock
    stepperE0.rms_current(E_UNLOCK_CURRENT_MA);

    // Partial unlock
    e_move(-E_PARTIAL_UNLOCK_DISTANCE, E_UNLOCK_FEEDRATE);
    planner.synchronize();
}

void PrusaToolChanger::open_head(PhysicalToolIndex tool) {
    // Sanity check: no tool should be thermally managed while we open the head
    IndxHotend::assert_thermally_managed_invariant(NoTool {});

    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/false);
    const float safe_y = info.dock_y + DOCK_SAFE_Y_OFFSET;
    const float unlock_y = info.dock_y + DOCK_UNLOCK_Y_OFFSET;

    const float travel_fr = limit_stealth_feedrate(OPEN_HEAD_TRAVEL_MOVE_MM_S);

    // Move to dock X, then to safe Y in front of dock
    move(info.dock_x, safe_y, travel_fr);
    planner.synchronize();

    // Approach unlock position
    move(info.dock_x, unlock_y, travel_fr);
    planner.synchronize();

    // Dwell for reliability
    (void)wait([]() { return false; }, DOCK_DWELL_MS);

    {
        EMotorGuard guard;
        // Increase E current for actual unlock
        stepperE0.rms_current(E_UNLOCK_CURRENT_MA);

        e_move(E_FULL_CLOSE_DISTANCE, E_LOCK_FEEDRATE); // Ensure fully closed to start with
        planner.synchronize();

        wiggle_and_partial_unlock();

        // Full open (no Y movement needed — no nozzle to release)
        e_move(-E_FULL_OPEN_DISTANCE, E_FULL_OPEN_FEEDRATE);
        planner.synchronize();
    }

    // Dwell for reliability
    (void)wait([]() { return false; }, DOCK_DWELL_MS);

    // Exit from dock area
    move(info.dock_x, safe_y, FAST_EXIT_FEEDRATE);
    planner.synchronize();

    head_open = true;

    log_info(PrusaToolChanger, "Head locking mechanism opened");
}

xy_pos_t PrusaToolChanger::tool_park_position(PhysicalToolIndex tool) {
    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/false);
    return xy_pos_t { .x = info.dock_x, .y = info.dock_y + DOCK_SAFE_Y_OFFSET };
}

bool PrusaToolChanger::park_procedure(PhysicalToolIndex tool) {
    // Invalidate nozzle presence data during park — the physical state is changing,
    // so the last reported result is stale until a fresh modbus read arrives.
    buddy::puppies::indx.invalidate_nozzle_data();

    IndxHotend::indx_tool(tool).hotend().stop_heating();

    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/false);
    const auto park_pos = tool_park_position(tool);

    // Move to dock X, then to safe Y in front of dock
    mapi::park({ .x = park_pos.x, .y = park_pos.y });
    planner.synchronize();

    // Approach unlock position
    move(park_pos.x, info.dock_y, DOCK_ENGAGE_FEEDRATE);
    planner.synchronize();

    {
        EMotorGuard guard;
        wiggle_and_partial_unlock();

        // Full open — nozzle is released
        e_move(-E_FULL_OPEN_DISTANCE, E_FULL_OPEN_FEEDRATE);
        planner.synchronize();
    }

    // Dwell for reliability
    (void)wait([]() { return false; }, DOCK_DWELL_MS);

    // Fast move to safe position
    move(park_pos.x, park_pos.y, SLOW_EXIT_FEEDRATE);
    planner.synchronize();

    buddy::puppies::indx.invalidate_nozzle_data();

    // Verify nozzle is gone (successfully released)
    if (!nozzle_check_disabled && !verify_nozzle_state(tool, false)) {
        return false;
    }

    return true;
}

void PrusaToolChanger::commit_park(PhysicalToolIndex previous_tool) {
    head_open = true;
    loadcell.Clear();
    log_info(PrusaToolChanger, "INDX Tool #%u parked successfully", previous_tool.to_raw());
    set_active_extruder(NoTool {});
    persist_last_picked_tool(NoTool {});
}

bool PrusaToolChanger::park(PhysicalToolIndex tool) {
    uint8_t max_retry_cnt = 2;

    // On bail-out, reconcile active_extruder with what the sensor sees.
    ScopeGuard committer = [&] {
        // bail_on_power_panic - the presence analysis must settle even after a user Stop.
        // Power-panic still short-circuits inside wait() so we don't drain PSU caps.
        if (!nozzle_check_disabled && verify_nozzle_state(tool, false, WaitMode::bail_on_power_panic)) {
            commit_park(tool);
        }
    };

    for (;;) {
        if (!ensure_safe_move()) {
            return false; // We cannot even home, abort the print
        }
        if (park_procedure(tool)) {
            break; // Successful park, nozzle verified gone
        }
        head_open = false;
        ensure_tool_homeable(tool);
        invalidate_xy_homing();

        ++park_fail_count;

        // Outside of a print there is no toolchange recovery UX to run
        // (the failure dialog assumes printing). Bail out
        // and let the caller — typically selftest — decide how to handle it.
        if (!marlin_server::is_printing()) {
            return false;
        }

        // Nozzle still detected -- first retry
        if (max_retry_cnt > 0) {
            log_warning(PrusaToolChanger, "Park procedure for tool #%u failed, retrying (remaining retries: %u)", tool.to_raw(), max_retry_cnt);
            --max_retry_cnt;
            continue;
        }

        // Nozzle still detected -- handle failure
        switch (apply_failure_action(handle_park_failure())) {
        case ToolchangeFailureAction::abort:
            return false;
        case ToolchangeFailureAction::retry:
            continue;
        }
        break;
    }

    committer.disarm();
    commit_park(tool);
    return true;
}

bool PrusaToolChanger::verify_nozzle_state(PhysicalToolIndex prev_tool, bool expect_present, WaitMode mode) {
    // Wait until nozzle presence confirms the expected post-pickup/park state.
    // This avoids failing on an early stale-but-valid sample from before the mechanical transition settled.
    // Note: a "stuck halfway" nozzle reads as `unknown` on the head side (decay between thresholds),
    // so it stays nullopt here and times out into the retry/abort recovery branch below.
    const bool data_ready = wait(
        [expect_present]() { return buddy::puppies::indx.get_nozzle_present() == std::optional<bool>(expect_present); },
        NOZZLE_VERIFY_TIMEOUT_MS,
        mode);

    if (data_ready) {
        log_info(PrusaToolChanger, "Nozzle verify after %s tool #%u: ok",
            expect_present ? "pickup" : "park", prev_tool.to_raw());
        return true;
    }

    if (expect_present) {
        log_warning(PrusaToolChanger, "Nozzle not detected after pickup of tool #%u", prev_tool.to_raw());
    } else {
        log_warning(PrusaToolChanger, "Nozzle still detected after park of tool #%u", prev_tool.to_raw());
    }
    return false;
}

PrusaToolChanger::ToolchangeFailureAction PrusaToolChanger::handle_toolchange_failure(
    PhaseNozzleMismatch main_phase,
    PhaseNozzleMismatch confirm_abort_phase) {

    if (gcode_exceptions().is_unwinding()) {
        return ToolchangeFailureAction::abort;
    }

    disable_xy_steppers();
    marlin_server::FSM_Holder fsm(main_phase);

    for (;;) {
        if (gcode_exceptions().is_unwinding()) {
            return ToolchangeFailureAction::abort;
        }
        const auto response = marlin_server::wait_for_response(main_phase);

        if (response == Response::Abort) {
            marlin_server::fsm_change(confirm_abort_phase);
            const auto confirm = marlin_server::wait_for_response(confirm_abort_phase);
            if (confirm == Response::Back) {
                marlin_server::fsm_change(main_phase);
                continue;
            }
            return ToolchangeFailureAction::abort;
        }

        // Response::Retry — home XY
        marlin_server::fsm_change(PhaseNozzleMismatch::homing);
        if (!ensure_safe_move()) {
            return ToolchangeFailureAction::abort;
        }

        return ToolchangeFailureAction::retry;
    }
}

PrusaToolChanger::ToolchangeFailureAction PrusaToolChanger::handle_park_failure() {
    return handle_toolchange_failure(
        PhaseNozzleMismatch::park_failed,
        PhaseNozzleMismatch::confirm_abort);
}

PrusaToolChanger::ToolchangeFailureAction PrusaToolChanger::handle_pickup_failure() {
    return handle_toolchange_failure(
        PhaseNozzleMismatch::pickup_failed,
        PhaseNozzleMismatch::confirm_abort);
}

PrusaToolChanger::ToolchangeFailureAction PrusaToolChanger::apply_failure_action(ToolchangeFailureAction action) {
    if (action == ToolchangeFailureAction::abort) {
        marlin_server::print_abort();
    }

    return action;
}

bool PrusaToolChanger::pickup_procedure(PhysicalToolIndex tool) {
    ensure_head_open(tool);

    const PrusaToolInfo &info = get_tool_info(tool, /*check_calibrated=*/true);
    const float safe_y = info.dock_y + DOCK_SAFE_Y_OFFSET;

    // Move to dock X, then to safe Y in front of dock
    mapi::park({ .x = info.dock_x, .y = safe_y });
    planner.synchronize();

    // Move into dock — nozzle enters head
    move(info.dock_x, info.dock_y, PICKUP_APPROACH_FEEDRATE);
    planner.synchronize();

    {
        EMotorGuard guard;

        // Increase E current for actual unlock
        stepperE0.rms_current(E_UNLOCK_CURRENT_MA);

        // Lock nozzle in head
        e_move(+E_FULL_CLOSE_DISTANCE, E_LOCK_FEEDRATE);
        planner.synchronize();

        // Engage the extruder hack - make sure that we don't retract before the head is fully locked
        buddy::indx_tool_lock_hack().rearm(Badge<PrusaToolChanger> {});
    }

    // Dwell for reliability
    (void)wait([]() { return false; }, DOCK_DWELL_MS);

    // Publish that the lock dwell is complete; PP can now continue from after_lock.
    phase_.store(ToolchangePhase::after_lock, std::memory_order_release);

    // Full exit to safe position
    move(info.dock_x, safe_y, SLOW_EXIT_FEEDRATE);
    planner.synchronize();

    buddy::puppies::indx.invalidate_nozzle_data();
    if (!nozzle_check_disabled && !verify_nozzle_state(tool, true)) {
        return false;
    }

    // at this point the tool is thermally managed
    IndxHotend::indx_tool(tool).hotend().start_heating();

    return true;
}

bool PrusaToolChanger::pickup(PhysicalToolIndex tool) {
    uint8_t max_retry_cnt = 1;
    for (;;) {
        if (!ensure_safe_move()) {
            return false; // We cannot even home, abort the print
        }
        if (pickup_procedure(tool)) {
            break; // success — nozzle verified, locked, and exited dock
        }
        head_open = false;
        ensure_tool_homeable(tool);
        invalidate_xy_homing();

        // Nozzle not detected — recovery path
        ++pickup_fail_count;
        IndxHotend::indx_tool(tool).hotend().stop_heating(); // prevent damage while we are trying to recover

        // Outside of a print there is no toolchange recovery UX to run
        // (the failure dialog assumes printing). Bail out
        // and let the caller — typically selftest — decide how to handle it.
        if (!marlin_server::is_printing()) {
            return false;
        }

        // Nozzle still detected -- first retry
        if (max_retry_cnt > 0) {
            log_warning(PrusaToolChanger, "Pickup procedure for tool #%u failed, retrying (remaining retries: %u)", tool.to_raw(), max_retry_cnt);
            --max_retry_cnt;
            continue;
        }

        // TODO: What if we are trying to pick unmapped tool? A fallback procedure rather than print pause would make more sense here

        switch (apply_failure_action(handle_pickup_failure())) {
        case ToolchangeFailureAction::abort:
            return false;
        case ToolchangeFailureAction::retry:
            continue;
        }
        break;
    }

    commit_pickup(tool, /*count_in_odometer=*/true, /*force_persist=*/false);
    return true;
}

void PrusaToolChanger::commit_pickup(PhysicalToolIndex tool, bool count_in_odometer, bool force_persist) {
    head_open = false;
    loadcell.Clear();

    log_info(PrusaToolChanger, "INDX Tool #%u picked successfully", tool.to_raw());
    if (count_in_odometer) {
        Odometer_s::instance().add_toolpick(tool);
    }

    set_active_extruder(tool);
    persist_last_picked_tool(tool, force_persist);
}

void PrusaToolChanger::final_tool_change_moves(const FinalToolChangeMoves &args) {
    // update return_position to the new working offset
    xyz_pos_t return_position = args.return_position + args.tool_offset_diff;
    // Prevent a move outside physical bounds
    apply_motion_limits(return_position);

    // Move back in XY direction
    if (args.return_type == tool_return_t::dock_backoff) {
        // After docking, back off in +Y to clear the dock area (e.g. for manual filament removal)
        mapi::park({ .y = current_position.y + DOCK_BACKOFF_Y_OFFSET });
    } else if (args.return_type > tool_return_t::no_return) {
        // Move back to the original (or adjusted) position.
        mapi::park({ .x = return_position.x, .y = return_position.y });
    }

    // Now move down in Z
    if (args.z_return) {
        set_bed_leveling_enabled(args.levelling_active); // Reenable MBL for this move
        if (current_position.z != return_position.z) {
            destination = current_position;
            destination.z = return_position.z;
            prepare_move_to(destination, Z_HOP_FEEDRATE_MM_S, {});
        }
    }

    // Wait for moves to finish
    planner.synchronize();
}

std::expected<void, PrusaToolChanger::BumpError> PrusaToolChanger::bump_to_dock(PhysicalToolIndex dock) {
    // Home if needed (mapi::park below requires XY homed)
    if (!ensure_safe_move()) {
        return std::unexpected<BumpError>(BumpError::unsafe_move);
    }
    planner.synchronize();

    mapi::park({ .z = mapi::ParkingPosition::AtLeast { .above_print = 2.0f } });

    // Get dock info
    const PrusaToolInfo &info = get_tool_info(dock, /*check_calibrated=*/false);

    // reduce maximum parking speed to improve reliability during constant toolchanging
    float target_fr = limit_stealth_feedrate(PARKING_FINAL_MAX_SPEED);
    const float safe_y = info.dock_y + DOCK_SAFE_Y_OFFSET;
    const auto move_distance = info.dock_y - safe_y;

    // go in front of the tool dock
    move(current_position.x, safe_y, target_fr);
    planner.synchronize();
    move(info.dock_x, safe_y, target_fr);
    planner.synchronize();

    // INDX_TODO: test with real hardware and make sure we dont break anything (possible to reduce currents and speeds)
    const bool hit = do_homing_move(Y_AXIS, move_distance, homing_feedrate(Y_AXIS)); // move down to the dock

    // We've hit endstops so back off a bit and warn user probably
    if (hit) {
        do_blocking_move_to_xy(info.dock_x, safe_y + 25.f, target_fr);
        log_warning(PrusaToolChanger, "Bump to dock #%u detected endstop hit, dock probably occupied", dock.to_raw());
        // Invalidate homing state to trigger rehome before next toolchaínge attempt, to be sure we have correct position of the dock in the future
        invalidate_xy_homing();
        return std::unexpected<BumpError>(BumpError::hit);
    }

    return std::expected<void, BumpError>();
}

void PrusaToolChanger::pick_tool_out_of_dock(PhysicalToolIndex dock) {
    // picking the tool from dock may be enough
    if (dock.is_enabled()) {
        if (tool_change(dock, tool_return_t::no_return, {}, tool_change_lift_t::full_lift, false)) {
            return; // dock was just emptied
        }
    }

    // we still dont know for sure
    if (!prusa_toolchanger.pick_any_tool(tool_return_t::no_return, {}, tool_change_lift_t::full_lift, false)) {
        fatal_error(ErrCode::ERR_MECHANICAL_TOOLCHANGER);
    }
    auto bump_res = prusa_toolchanger.bump_to_dock(dock);
    if (bump_res.has_value()) {
        return; // dock is empty
    }
    switch (bump_res.error()) {
    case PrusaToolChanger::BumpError::hit:
        fatal_error(ErrCode::ERR_MECHANICAL_UNEXPECTED_TOOL);
    case PrusaToolChanger::BumpError::unsafe_move:
        fatal_error("Homing failed.", "PrusaToolChanger");
    }
}

void PrusaToolChanger::persist_last_picked_tool(std::variant<PhysicalToolIndex, NoTool> tool, bool override_always) {
    // Single-tool print: the tool can't change, so leave EEPROM frozen at the value set at print start.
    if (!override_always && freeze_last_picked_tool_ && marlin_server::is_printing()) {
        return;
    }
    // If we are not printing, we need to save the no tool picked to eeprom
    if (override_always || !marlin_server::is_printing()) {
        config_store().set_indx_last_picked_tool(tool);
        config_store().indx_last_picked_tool_valid.set(true);
    } else if (config_store().indx_last_picked_tool_valid.get()) {
        // Tool park while printing && not yet invalidated -> invalidate
        config_store().indx_last_picked_tool_valid.set(false);
    }
}

bool PrusaToolChanger::recover_pp_toolchange(
    const ToolchangeReturnData &rd,
    std::variant<PhysicalToolIndex, NoTool> active_tool,
    ToolchangePhase phase) {

    switch (phase) {
    case ToolchangePhase::before_lock:
        return recover_before_lock(rd, active_tool);
    case ToolchangePhase::after_lock:
        return finish_pickup_after_lock(rd);
    case ToolchangePhase::none:
        bsod("recover_pp_toolchange called with phase none");
    }
    bsod_unreachable();
}

bool PrusaToolChanger::recover_before_lock(
    const ToolchangeReturnData &rd,
    std::variant<PhysicalToolIndex, NoTool> active_tool) {

    // Bring the planner's view of the active tool in line with the held tool,
    // then let tool_change re-run the full sequence from the top.
    set_active_extruder(active_tool);
    return tool_change(rd.tool, rd.return_type, rd.return_pos.asNative());
}

bool PrusaToolChanger::finish_pickup_after_lock(const ToolchangeReturnData &rd) {
    // after_lock implies a physical tool is being picked; NoTool target never reaches the lock phase.
    if (!std::holds_alternative<PhysicalToolIndex>(rd.tool)) {
        bsod("finish_pickup_after_lock with NoTool target");
    }
    const PhysicalToolIndex tool = std::get<PhysicalToolIndex>(rd.tool);

    if (!ensure_safe_move()) {
        return false;
    }

    buddy::puppies::indx.invalidate_nozzle_data();
    if (!nozzle_check_disabled && !verify_nozzle_state(tool, true)) {
        return false;
    }

    // Odometer increment was already counted before PP — skip it here.
    commit_pickup(tool, /*count_in_odometer=*/false, /*force_persist=*/true);

    // MBL has not been re-enabled yet during PP resume. The Z portion of the
    // offset diff was physically applied before PP (the z_shifts run before
    // the after_lock store). The XY portion was never physically applied —
    // it is implicit in rd.return_pos.asNative() because PP restore set
    // hotend_currently_applied_offset to the new tool's offset, so the
    // logical→native conversion already produces the adjusted native target.
    // Pass 0 to skip a redundant in-place adjustment.
    final_tool_change_moves({
        .return_position = rd.return_pos.asNative(),
        .return_type = rd.return_type,
        .levelling_active = false,
        .z_return = true,
        .tool_offset_diff = xyz_pos_t { 0, 0, 0 },
    });
    return true;
}
