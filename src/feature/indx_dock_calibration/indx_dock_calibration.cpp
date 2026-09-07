#include "indx_dock_calibration.hpp"
#include <test_result.hpp>

#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/mapi/calibration_preamble.hpp>
#include <gcode/gcode.h>
#include <marlin_server.hpp>
#include <module/endstops.h>
#include <module/motion.h>
#include <module/planner.h>
#include <module/stepper.h>
#include <module/stepper/indirection.h>
#include <module/prusa/toolchanger.h>
#include <module/prusa/corexy_transform.hpp>
#include <puppies/INDX.hpp>
#include "dock_calibration_data.hpp"

#include <bitset>
#include <tool_index.hpp>
#include <logging/log.hpp>
#include <config_store/store_instance.hpp>
#include <common/selftest_result.hpp>
#include <selftest/selftest_invocation.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_DEF(DockCalibration, logging::Severity::info);

using marlin_server::wait_for_response;

namespace indx_dock_calibration {

/// How far to move in Y to clear the dock after measurement [mm]
static constexpr float DOCK_EXIT_Y_MM = 30.0f;

/// How far to move in -X to clear the last dock before homing [mm]
static constexpr float LAST_DOCK_EXIT_X_MM = 40.0f;

/// Distance to bump into the docks before measurnig
static constexpr float DOCK_BUMP_MM = 10;

/// Back off before bumping [mm]
/// Homing moves don't trigger if they're already at the end
static constexpr float DOCK_BUMP_BACKOFF_MM = 2;

/// Serialize dock index into PhaseData (4 bytes)
static fsm::PhaseData serialize_dock_data(PhysicalToolIndex tool) {
    fsm::PhaseData data {};
    data[0] = tool.to_raw();
    return data;
}

class DockCalibrationWizard {
public:
    void run() {
        const auto result = run_inner();

        // Store calibration result
        auto sr = config_store().selftest_result.get();
        switch (result) {
        case Result::success:
            sr.set_dock_offset(PhysicalToolIndex::from_raw(0), TestResult::passed);
            break;
        case Result::failed:
            sr.set_dock_offset(PhysicalToolIndex::from_raw(0), TestResult::failed);
            break;
        case Result::aborted:
            // Aborting still counts as a pass if at least one dock is already calibrated
            // (the calibration is usable); otherwise leave the result unchanged.
            if (config_store().indx_dock_calibrated_mask.get().any()) {
                sr.set_dock_offset(PhysicalToolIndex::from_raw(0), TestResult::passed);
            } else {
                selftest_invocation::mark_aborted();
            }
            break;
        }
        config_store().selftest_result.set(sr);

        // Disable motors at the end of the wizard
        disable_XY();
        disable_Z();

        if (result == Result::success) {
            fsm_change(PhaseDockCalibration::calibration_success);
            wait_for_response(PhaseDockCalibration::calibration_success);
        }
    }

private:
    marlin_server::FSM_Holder holder { PhaseDockCalibration::intro };

    void fsm_change(PhaseDockCalibration phase, fsm::PhaseData data = {}) {
        marlin_server::fsm_change(phase, data);
    }

    enum class Result {
        success,
        failed,
        aborted,
    };

    /// Bitmask of docks selected for calibration
    std::bitset<PhysicalToolIndex::count> selected_docks;
    static_assert(PhysicalToolIndex::count <= 8, "dock actions are transferred as a DockSelection via FSMResponseVariant");

    Result run_inner() {
        // Intro
        fsm_change(PhaseDockCalibration::intro);
        if (wait_for_response(PhaseDockCalibration::intro) == Response::Abort) {
            return Result::aborted;
        }

        // A picked tool gets parked automatically during the preamble (to the default dock
        // position when its dock is not calibrated yet); ask for manual removal only when
        // a nozzle is detected but the firmware doesn't know which tool it is
        auto picked_tool = PhysicalToolIndex::currently_selected_opt();
        const auto nozzle_present = buddy::puppies::indx.get_nozzle_present();
        if (picked_tool && nozzle_present == std::optional(false)) {
            // The sensor definitively says the head is empty — the picked-tool state is stale
            log_warning(DockCalibration, "Tool #%u believed picked but no nozzle detected, correcting to no tool", picked_tool->to_raw());
            prusa_toolchanger.set_active_extruder(NoTool {});
            prusa_toolchanger.persist_last_picked_tool(NoTool {}, /*override_always=*/true);
            picked_tool.reset();
        }
        if (!picked_tool && nozzle_present.value_or(false)) {
            fsm_change(PhaseDockCalibration::remove_tool);
            if (wait_for_response(PhaseDockCalibration::remove_tool) == Response::Abort) {
                return Result::aborted;
            }
            bsod_unreachable();
        }

        // Select how many docks the user has
        uint8_t dock_count = PhysicalToolIndex::count;
        // When the user picks a known dock count (4 or 8), default to calibrating
        // every dock; when they pick "Other", let them refine the default selection
        // (only uncalibrated docks pre-selected).
        bool preselect_all = false;
        fsm_change(PhaseDockCalibration::select_dock_count);
        {
            const auto response = wait_for_response(PhaseDockCalibration::select_dock_count);
            switch (response) {
            case Response::Docks4:
                dock_count = 4;
                preselect_all = true;
                break;
            case Response::Docks8:
                dock_count = 8;
                preselect_all = true;
                break;
            case Response::Other:
                dock_count = 8;
                break;
            default:
                bsod_unreachable();
            }
        }

        // Select which docks to calibrate — pass dock_count and preselect_all via PhaseData
        {
            fsm::PhaseData data {};
            data[0] = dock_count;
            data[1] = preselect_all ? 1 : 0;
            fsm_change(PhaseDockCalibration::select_docks, data);
        }
        {
            const auto response = marlin_server::wait_for_response_variant(PhaseDockCalibration::select_docks);
            const auto *sel = response.value_maybe<DockSelection>();
            if (!sel) {
                return Result::aborted;
            }

            // Invalidate selected docks
            if (sel->invalidate) {
                auto calibrated_mask = config_store().indx_dock_calibrated_mask.get();
                for (auto tool : PhysicalToolIndex::all()) {
                    if (sel->invalidate & (1 << tool.to_raw())) {
                        calibrated_mask.reset(tool.to_raw());
                    }
                }
                config_store().indx_dock_calibrated_mask.set(calibrated_mask);
            }

            selected_docks = std::bitset<PhysicalToolIndex::count>(sel->calibrate);
            if (selected_docks.none()) {
                return Result::aborted;
            }
        }

        const mapi::CalibrationPreamble preamble {
            .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_parked,
            .on_step = [&](mapi::CalibrationPreamble::Step step) {
                switch (step) {
                case mapi::CalibrationPreamble::Step::moving_away:
                    fsm_change(PhaseDockCalibration::moving_away);
                    break;
                case mapi::CalibrationPreamble::Step::parking_tool:
                    fsm_change(PhaseDockCalibration::parking_tool);
                    break;
                case mapi::CalibrationPreamble::Step::homing:
                    fsm_change(PhaseDockCalibration::homing);
                    break;
                case mapi::CalibrationPreamble::Step::picking_tool:
                    bsod_unreachable();
                }
            },
        };

        // Park the picked tool (if any) so the head is empty for calibration
        const bool preamble_ok = preamble.run();

        // A failed park reconciles the picked-tool state with the sensor on bail-out, so no tool
        // picked anymore means the park succeeded and only its verification hiccuped — carry on
        if (!preamble_ok && PhysicalToolIndex::currently_selected_opt()) {
            // Park failed — the tool is still on the head; ask the user to remove it manually
            fsm_change(PhaseDockCalibration::remove_tool);
            if (wait_for_response(PhaseDockCalibration::remove_tool) == Response::Abort) {
                return Result::aborted;
            }
            bsod_unreachable();
        }

        // Ensure head locking mechanism is open (no tool present)
        prusa_toolchanger.ensure_head_open();

        // Ask user to tighten the silver screws on each dock before measuring
        fsm_change(PhaseDockCalibration::tighten_silver_screws);
        if (wait_for_response(PhaseDockCalibration::tighten_silver_screws) == Response::Abort) {
            return Result::aborted;
        }

        // Calibrate selected docks
        for (auto tool : PhysicalToolIndex::all()) {
            if (!selected_docks.test(tool.to_raw())) {
                continue;
            }
            const auto r = calibrate_dock(tool);
            if (r != Result::success) {
                return r;
            }
        }

        // Ask user to loosen each bolt by exactly one turn after calibration
        fsm_change(PhaseDockCalibration::loosen_each_bolt);
        wait_for_response(PhaseDockCalibration::loosen_each_bolt);

        return Result::success;
    }

    Result calibrate_dock(PhysicalToolIndex tool) {
        phase_stepping::EnsureSuitableForHoming phstep_guard;
        TemporaryGlobalEndstopsState endstops_guard(true);

        for (;;) { // Retry loop for the entire dock calibration
            if (axes_home_level.is_homed({ X_AXIS, Y_AXIS }, AxisHomeLevel::imprecise)) {
                // Move in front of the dock so the user has a shorter distance to adjust
                // We may not be homed after a retry
                current_position.x = PrusaToolChanger::DOCK_DEFAULT_X_MM[tool];
                current_position.y = PrusaToolChanger::DOCK_DEFAULT_Y_MM + PrusaToolChanger::DOCK_SAFE_Y_OFFSET;
                line_to_current_position(PrusaToolChanger::TRAVEL_MOVE_MM_S);
                planner.synchronize();
            }

            // Position and confirm loop — user can go back to reposition
            for (;;) {
                // Disable XY motors so user can move the head
                planner.synchronize();
                disable_XY();

                // Ask user to position head at dock
                fsm_change(PhaseDockCalibration::ask_position_dock, serialize_dock_data(tool));
                if (wait_for_response(PhaseDockCalibration::ask_position_dock) == Response::Abort) {
                    return Result::aborted;
                }

                // Re-enable motors to lock the head in place
                enable_XY();

                // Ask user to confirm position before moving
                fsm_change(PhaseDockCalibration::lock_position, serialize_dock_data(tool));
                const auto response = wait_for_response(PhaseDockCalibration::lock_position);
                if (response == Response::Abort) {
                    return Result::aborted;
                }
                if (response == Response::Continue) {
                    break; // Position confirmed, proceed to measurement
                }
                // Response::Back — loop back to repositioning
            }

            // Measuring
            fsm_change(PhaseDockCalibration::measuring, serialize_dock_data(tool));

            // Back off a bit
            const bool back_off_hit = do_homing_move(Y_AXIS, DOCK_BUMP_BACKOFF_MM, homing_feedrate(Y_AXIS));

            if (back_off_hit) {
                // Hitting something during a backoff would be very unexpected
                // Ask the user to position the head again if this happens
                continue;
            }

            // Bump further in the dock
            // The dock unlocking magnet pushes the head out of the docks when the motors are off,
            // so if the user stops pressing before the motors are engaged, we would get a wrong reading
            const bool dock_bump_hit = do_homing_move(Y_AXIS, -(DOCK_BUMP_MM + DOCK_BUMP_BACKOFF_MM), homing_feedrate(Y_AXIS));

            // Wait a bit after the bump, the sudden move-forth-then-back might scare the user
            // Don't be afraid, users, we love you!
            GcodeSuite::dwell(200);

            if (!dock_bump_hit) {
                // If the homing move didn't hit anything, it means that the head is completely at the wrong position
                // Just move back and ask the user to position the head again
                do_homing_move(Y_AXIS, DOCK_BUMP_MM, homing_feedrate(Y_AXIS));
                continue;
            }

            // Reset current position to (0,0) as reference point for measurement
            planner.synchronize();
            current_position.x = 0;
            current_position.y = 0;
            sync_plan_position();

            // Record stepper positions before homing
            const ab_steps_t position_before = { { {
                .x = stepper.position_from_startup(AxisEnum::A_AXIS),
                .y = stepper.position_from_startup(AxisEnum::B_AXIS),
            } } };

            // Move Y to clear the dock before homing
            current_position.y += DOCK_EXIT_Y_MM;
            line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);
            planner.synchronize();

            // Dock 8 (index 7): move away in -X to avoid hardware collision with motor during homing
            if (tool.to_raw() == PhysicalToolIndex::count - 1) {
                current_position.x -= LAST_DOCK_EXIT_X_MM;
                line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);
                planner.synchronize();
            }

            // Home XY (z_raise=0: Z is at the bottom, no need for Z clearance)
            GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .precise = true });

            // Record stepper positions after homing
            const ab_steps_t position_after = { { {
                .x = stepper.position_from_startup(AxisEnum::A_AXIS),
                .y = stepper.position_from_startup(AxisEnum::B_AXIS),
            } } };

            // Convert AB stepper difference to XY mm (CoreXY transform)
            // Dock position = home position + (position_before - position_after)
            const MachinePosXY diff = corexy_ab_to_xy(position_before - position_after);

            const PrusaToolInfo measured = {
                .dock_x = diff.x + current_position.x,
                .dock_y = diff.y + current_position.y + 1.0f, // Add 1mm to compensate for the fact that the tool is not perfectly flush with the dock surface
            };

            // Validate
            if (!prusa_toolchanger.is_tool_info_valid(tool, measured)) {
                log_error(DockCalibration,
                    "Dock %u position %.1f, %.1f out of bounds",
                    tool.to_raw(), static_cast<double>(measured.dock_x), static_cast<double>(measured.dock_y));

                // Move in front of the dock before showing the error
                current_position.x = PrusaToolChanger::DOCK_DEFAULT_X_MM[tool];
                current_position.y = PrusaToolChanger::DOCK_DEFAULT_Y_MM + PrusaToolChanger::DOCK_SAFE_Y_OFFSET;
                line_to_current_position(PrusaToolChanger::TRAVEL_MOVE_MM_S);
                planner.synchronize();

                // Disable XY motors so the user can move the head
                disable_XY();

                fsm_change(PhaseDockCalibration::calibration_failed,
                    DockCalibrationFailedData { tool, measured.dock_x, measured.dock_y }.serialize());
                if (wait_for_response(PhaseDockCalibration::calibration_failed) == Response::Retry) {
                    continue; // Retry this dock from the beginning
                }

                // Invalidate calibration for this dock since measurement failed
                auto calibrated_mask = config_store().indx_dock_calibrated_mask.get();
                calibrated_mask.reset(tool.to_raw());
                config_store().indx_dock_calibrated_mask.set(calibrated_mask);

                return Result::failed;
            }

            // Apply
            prusa_toolchanger.set_tool_info(tool, measured);

            // Mark dock as calibrated in persistent store
            auto calibrated_mask = config_store().indx_dock_calibrated_mask.get();
            calibrated_mask.set(tool.to_raw());
            config_store().indx_dock_calibrated_mask.set(calibrated_mask);

            // Persist dock positions after each successful calibration
            prusa_toolchanger.save_tool_info();

            log_info(DockCalibration,
                "Dock %u calibrated: x=%.2f y=%.2f",
                tool.to_raw(), static_cast<double>(measured.dock_x), static_cast<double>(measured.dock_y));

            return Result::success;
        }
    }
};

void run() {
    DockCalibrationWizard wizard;
    wizard.run();
}

} // namespace indx_dock_calibration
