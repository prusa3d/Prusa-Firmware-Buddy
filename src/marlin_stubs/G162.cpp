#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include <module/endstops.h>

#include "PrusaGcodeSuite.hpp"
#include <stdint.h>
#include "bsod.h"
#include "calibration_z.hpp"
#include "client_response.hpp"
#include "printers.h"
#include <mapi/parking.hpp>
#include <marlin_server.hpp>
#include <raii/auto_restore.hpp>
#include <utils/progress.hpp>

#include <option/has_loadcell.h>
#if HAS_LOADCELL()
    #include "loadcell.hpp"
#endif

#include <feature/print_status_message/print_status_message.hpp>
#include <feature/print_status_message/print_status_message_guard.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include <logging/log.hpp>

LOG_COMPONENT_REF(Selftest);

namespace {

constexpr feedRate_t align_axis_feedrate = 15.f; // mm/s
constexpr float extra_height = 5.f; // mm

#if PRINTER_IS_PRUSA_XL()

/// Feedrate of the final push into the top hard stop, on which the motors skip to align themselves
constexpr feedRate_t align_axis_slow_feedrate = 4.f; // mm/s

/// Nozzle clearance left behind, dock calibration crosses the bed right after this
constexpr float safe_clearance = 10.f; // mm

#else
/// For detecting, if we travel the whole Z axis distance, or if we crashed into an obstacle
constexpr float z_axis_leeway = 10.f; // mm
#endif

}; // namespace

#if PRINTER_IS_PRUSA_XL()

static void ensure_tool_is_in_dock_area() {
    const auto tool = PhysicalToolIndex::currently_selected_opt();
    if (!tool.has_value()) {
        return;
    }
    if (prusa_toolchanger.is_toolchanger_enabled()) {
        if (!prusa_toolchanger.tool_change(NoTool {}, tool_return_t::no_return, {}, tool_change_lift_t::full_lift, false)) {
            fatal_error(ErrCode::ERR_MECHANICAL_TOOLCHANGER);
        }
    } else {
        // single-tool have no docks, just move to the dock area
        // WARN: the bed may be at the top position, we need to ensure safe moves in X, Y
        const auto dock_pos = mapi::ParkingPosition {
            .x = 20.f, // arbitrary position not colliding with steppers
            .y = static_cast<float>(Y_MAX_POS - 1),
            .z = mapi::ParkingPosition::AtLeast { .absolute = extra_height }
        };
        GcodeSuite::G28_no_parser(true, true, false, { .only_if_needed = true, .z_raise = extra_height, .precise = false });
        mapi::park(dock_pos);
    }
}

void selftest::calib_Z([[maybe_unused]] bool move_down_after) {
    marlin_server::fsm_change(PhasesSelftest::CalibZ);

    // backup original acceleration/feedrates and reset defaults for calibration
    Temporary_Reset_Motion_Parameters mp;

    // mark test as failed (so it will be failed after reset - disconnected cables can cause rsod)
    auto result = config_store().selftest_result.get();
    result.set_zalign(TestResult::failed);
    config_store().selftest_result.set(result);

    // prevent collision with tool
    ensure_tool_is_in_dock_area();

    // move bed up to the end of Z axis
    const auto z_axis_length = Z_MAX_POS - Z_MIN_POS;
    if (!do_homing_move(Z_AXIS, -(z_axis_length + extra_height), HOMING_FEEDRATE_INVERTED_Z, false, false)
        && !planner.draining()) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    current_position.z = Z_MIN_POS;
    sync_plan_position();

    // The position above is fabricated and the push below deliberately targets past the
    // software limit. Drop the homed flag first, otherwise apply_motion_limits crops it.
    set_axis_is_not_at_home(Z_AXIS);

    // Lower the bed and repeat the push slowly to align the motors
    do_blocking_move_to_z(Z_MIN_POS + extra_height, align_axis_feedrate);
    do_blocking_move_to_z(Z_MIN_POS - extra_height, align_axis_slow_feedrate);

    // Back off the hard stop, the axis is left unhomed and Z_MIN_POS is past nozzle contact
    do_blocking_move_to_z(Z_MIN_POS + safe_clearance, align_axis_feedrate);

    // Store Z aligned
    result.set_zalign(TestResult::passed);
    config_store().selftest_result.set(result);
}
#else
namespace {
constexpr float AFTER_Z_CALIB_Z_POS = 50;

void safe_move_down() {
    DEPLOY_PROBE();

    // Move to AFTER_Z_CALIB_Z_POS with Z endstop enabled
    float target_Z = AFTER_Z_CALIB_Z_POS - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.z);

    if (do_homing_move(AxisEnum::Z_AXIS, target_Z - current_position.z, HOMING_FEEDRATE_INVERTED_Z)) {
        // endstop triggered, raise the nozzle
        move_z_after_probing();
    }

    STOW_PROBE();
}

/// @brief Align the Z axis by forcing motors past endstops
/// @param move_down_after move back after aligning
/// @param prevent_collision REQUIRES Z AXIS TO BE HOMED. Measures the axis to prevent aligning being done against
/// @return true - Z aligned, false - Z align didn't finish
bool calib_Z_no_fsm(bool move_down_after, bool prevent_collision) {
    // If we want to prevent collision, Z axis needs to be measured
    // prevent_collision => Z homed
    debug_assert(!prevent_collision || axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise));

    // mark test as failed (so it will be failed after reset - disconnected cables can cause rsod)
    auto result = config_store().selftest_result.get();
    result.set_zalign(TestResult::failed);
    config_store().selftest_result.set(result);

    // backup original acceleration/feedrates and reset defaults for calibration
    Temporary_Reset_Motion_Parameters mp;

    // Z axis lift
    endstops.enable(true); // Stall endstops need to be enabled manually as in G28
    if (!do_homing_move(Z_AXIS, 1.5f * max_length(Z_AXIS), HOMING_FEEDRATE_INVERTED_Z, false, false)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    endstops.not_homing();

    const bool lift_blocked = prevent_collision && (current_position.z < (Z_MAX_POS - z_axis_leeway));
    if (lift_blocked) {
        log_warning(Selftest, "Z-align lift blocked at %.1f mm (expected at least %.1f mm)", (double)current_position.z, (double)(Z_MAX_POS - z_axis_leeway));

        // We just pushed against a object, move, so that user can remove it.
        do_blocking_move_to_z(current_position.z - 20, align_axis_feedrate);
    } else {
        current_position.z = Z_MAX_POS; // This position is fabricated and not reliable -> set as unhomed
        sync_plan_position();
        set_axis_is_not_at_home(Z_AXIS); // this is also needed to push past endstops

        // push both Z axis few mm over HW limit to align motors
        const float target_Z = Z_MAX_POS + (extra_height * 3);

        do_blocking_move_to_z(target_Z, align_axis_feedrate);
        current_position.z = Z_MAX_POS;
        sync_plan_position();

        // Store Z aligned
        result.set_zalign(TestResult::passed);
        config_store().selftest_result.set(result);

        // move a little bit back to stabilize the motors
        do_blocking_move_to_z(current_position.z - 1, align_axis_feedrate);
    }

    if (move_down_after) {
        safe_move_down();
    }

    return !lift_blocked;
}
} // namespace

void selftest::calib_Z(bool move_down_after) {
    marlin_server::fsm_change(PhasesSelftest::CalibZ);
    std::ignore = calib_Z_no_fsm(move_down_after, false);
}

    #if HAS_UNEVEN_BED_DETECTION()
bool selftest::auto_calib_Z(bool move_down_after) {
    // ensure Z axis is homed

    PrintStatusMessageGuard statusGuard;
    statusGuard.update<PrintStatusMessage::bed_aligning>({});

    GcodeSuite::G28_no_parser(false, false, true, { .only_if_needed = true, .precise = false });
    return calib_Z_no_fsm(move_down_after, true);
}
    #endif // HAS_UNEVEN_BED_DETECTION()

#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G162: Z Calibration <a href="https://reprap.org/wiki/G-code#G162:_Home_axes_to_maximum">G162: Home axes to maximum</a>
 *
 *#### Usage
 *
 *    G162 [ Z ]
 *
 *#### Parameters
 *
 *  - `Z` - Calibrate Z axis
 */

void PrusaGcodeSuite::G162() {
    if (parser.seen('Z')) {
#if HAS_PHASE_STEPPING()
        phase_stepping::EnsureDisabled ps_disabler;
#endif
        marlin_server::FSM_Holder holder { PhasesSelftest::CalibZ };
        selftest::calib_Z(PRINTER_IS_PRUSA_iX() ? false : true);
    }
}

/** @}*/
