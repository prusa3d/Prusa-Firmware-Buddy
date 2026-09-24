/// @file
#include "indx_gantry_squareness.hpp"

#include <common/fsm_base_types.hpp>
#include <common/mapi/calibration_preamble.hpp>
#include <common/mapi/parking.hpp>
#include <config_store/store_instance.hpp>
#include <gcode/gcode.h>
#include <marlin_server.hpp>
#include <marlin_server_types/fsm/gantry_squareness_phases.hpp>
#include <metric.h>
#include <module/motion.h>
#include <module/planner.h>
#include <module/stepper/indirection.h>
#include <module/prusa/toolchanger.h>
#include <raii/scope_guard.hpp>
#include <selftest/selftest_invocation.hpp>
#include <test_result.hpp>
#include <tool_index.hpp>

#include <logging/log.hpp>

#include <cmath>
#include <optional>

LOG_COMPONENT_DEF(GantrySquareness, logging::Severity::info);

METRIC_DEF(metric_gantry_squareness, "gantry_squareness", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);

namespace indx_gantry_squareness {

namespace {

    /// The outermost docks used as the two measurement points (even if tools are not present, thats ok)
    const PhysicalToolIndex left_dock = PhysicalToolIndex::from_raw(0);
    const PhysicalToolIndex right_dock = PhysicalToolIndex::from_raw(PhysicalToolIndex::count - 1);

    /// Extra travel past the nominal dock depth so the homing move surely stalls [mm]
    constexpr float PROBE_OVERTRAVEL_MM = 6.0f;

    /// How far to back away from the docks, so the user has room to handle the nozzles [mm]
    constexpr float DOCK_CLEARANCE_Y_MM = 50.0f;

    /// Back away from the docks so the user can reach the nozzles
    void back_away_from_docks() {
        const MachinePosXYZE pos = current_machine_position();
        line_to_machine_pos(pos.with_y(pos.y + DOCK_CLEARANCE_Y_MM), PrusaToolChanger::SLOW_MOVE_MM_S);
        planner.synchronize();
    }

    struct Measurement {
        bool hit;
        float y; ///< Machine Y of the stall stop
    };

    /// Drive the empty head into the dock until it stalls and report where that happened.
    Measurement measure_dock(PhysicalToolIndex tool) {
        // Travel in front of the dock (tool_park keeps us clear of the nozzle cleaner)
        mapi::park(mapi::get_parking_position(mapi::ParkPosition::tool_park, tool));
        planner.synchronize();

        const bool hit = do_homing_move(Y_AXIS, -(PrusaToolChanger::DOCK_SAFE_Y_OFFSET + PROBE_OVERTRAVEL_MM), homing_feedrate(Y_AXIS));

        // do_homing_move syncs the position from the actual stepper counters
        const Measurement result {
            .hit = hit,
            .y = current_machine_position().y,
        };

        // Back out of the dock
        const float dock_front_y = prusa_toolchanger.get_tool_dock_position(tool, /*check_calibrated=*/false).y;
        line_to_machine_pos(current_machine_position().with_y(dock_front_y), PrusaToolChanger::SLOW_MOVE_MM_S);
        planner.synchronize();

        log_info(GantrySquareness, "Dock %u: hit=%u y=%.3f", tool.display_index(), hit, static_cast<double>(result.y));
        return result;
    }

    /// Park the picked tool - it may belong to a dock the user is about to empty.
    bool prepare() {
        const bool tool_picked = PhysicalToolIndex::currently_selected_opt().has_value();

        const mapi::CalibrationPreamble preamble {
            .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_parked,
            .on_step = [](mapi::CalibrationPreamble::Step) {},
        };
        if (!preamble.run()) {
            log_error(GantrySquareness, "Parking failed");
            return false;
        }

        // Parking leaves the head right in front of the dock
        if (tool_picked) {
            back_away_from_docks();
        }
        return true;
    }

    /// Measure the gantry skew: home XY (imprecise) and stall the empty head
    /// against both outermost docks; dY of the two stops is the skew over their span.
    /// Expects prepare() done and the outermost docks empty of nozzles.
    /// @return skew (right minus left) [mm], nullopt on failure (reason on serial)
    std::optional<float> measure() {
        // Imprecise homing suffices - the reference cancels out in the dock difference
        if (!GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .precise = false })) {
            log_error(GantrySquareness, "Homing failed");
            return std::nullopt;
        }

        const Measurement left = measure_dock(left_dock);
        const Measurement right = measure_dock(right_dock);

        // Back away and release the motors, so the user can move the gantry
        // by hand while reinserting the nozzles
        back_away_from_docks();
        disable_XY();

        if (!left.hit || !right.hit) {
            log_error(GantrySquareness, "Dock %u did not stall", (left.hit ? right_dock : left_dock).display_index());
            return std::nullopt;
        }

        log_info(GantrySquareness, "left_y=%.3f right_y=%.3f dY=%.3f",
            static_cast<double>(left.y), static_cast<double>(right.y),
            static_cast<double>(right.y - left.y));
        metric_record_custom(&metric_gantry_squareness, " left_y=%.3f,right_y=%.3f,dy=%.3f",
            static_cast<double>(left.y), static_cast<double>(right.y),
            static_cast<double>(right.y - left.y));
        return right.y - left.y;
    }

} // namespace

void run_silent() {
    if (prepare()) {
        measure();
    }
}

void run_wizard() {
    marlin_server::FSM_Holder holder { PhaseGantrySquareness::intro };

    // Every exit is an abort, except passing the check
    ScopeGuard abort_guard = [] { selftest_invocation::mark_aborted(); };

    if (marlin_server::wait_for_response(PhaseGantrySquareness::intro) != Response::Continue) {
        return;
    }

    bool docks_emptied = false;

    // Once the user empties the docks, ask to refill them on every exit path
    ScopeGuard reinsert_guard = [&docks_emptied] {
        if (docks_emptied) {
            marlin_server::fsm_change(PhaseGantrySquareness::reinsert_nozzles);
            marlin_server::wait_for_response(PhaseGantrySquareness::reinsert_nozzles);
        }
    };
    while (true) {
        marlin_server::fsm_change(PhaseGantrySquareness::preparing);
        SquarenessData data {};
        if (prepare()) {
            if (!docks_emptied) {
                marlin_server::fsm_change(PhaseGantrySquareness::remove_nozzles);
                if (marlin_server::wait_for_response(PhaseGantrySquareness::remove_nozzles) != Response::Continue) {
                    return;
                }
                docks_emptied = true;
            }

            marlin_server::fsm_change(PhaseGantrySquareness::measuring);
            const std::optional<float> dy = measure();

            // Store right away - don't depend on the user confirming the result screen
            const bool passed = dy.has_value() && std::abs(*dy) <= dy_limit_mm;
            config_store().selftest_result_gantry_squareness.set(passed ? TestResult::passed : TestResult::failed);

            data = SquarenessData {
                .dy_um = static_cast<int16_t>(std::lroundf(dy.value_or(0.f) * 1000.f)),
                .valid = dy.has_value(),
            };
            if (passed) {
                marlin_server::fsm_change(PhaseGantrySquareness::result_ok, fsm::serialize_data(data));
                marlin_server::wait_for_response(PhaseGantrySquareness::result_ok);
                abort_guard.disarm();
                return;
            }
        }

        // Parking failure lands here too (valid == false, stored result untouched)
        marlin_server::fsm_change(PhaseGantrySquareness::result_failed, fsm::serialize_data(data));
        if (marlin_server::wait_for_response(PhaseGantrySquareness::result_failed) != Response::Retry) {
            return;
        }
    }
}

} // namespace indx_gantry_squareness
