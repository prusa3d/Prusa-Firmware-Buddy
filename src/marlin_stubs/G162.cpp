#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include <module/endstops.h>

#include "PrusaGcodeSuite.hpp"
#include <stdint.h>
#include "bsod.h"
#include "z_calibration_fsm.hpp"
#include "calibration_z.hpp"
#include "client_response.hpp"
#include "printers.h"

#include <option/has_loadcell.h>
#if HAS_LOADCELL()
    #include "loadcell.hpp"
#endif

static constexpr feedRate_t Z_CALIB_ALIGN_AXIS_FEEDRATE = 15.f; // mm/s
static constexpr float Z_CALIB_EXTRA_HIGHT = 5.f; // mm

#if PRINTER_IS_PRUSA_XL()
    #include <module/prusa/toolchanger.h>

void selftest::calib_Z([[maybe_unused]] bool move_down_after) {
    marlin_server::fsm_change(PhasesSelftest::CalibZ);

    // backup original acceleration/feedrates and reset defaults for calibration
    Temporary_Reset_Motion_Parameters mp;

    // Home XY first
    if (axis_unhomed_error(_BV(X_AXIS) | _BV(Y_AXIS))) {
        if (!GcodeSuite::G28_no_parser(false, 0, false, true, true, false)) {
            return; // This can happen only during print, homing recovery should follow
        }
    }

    // Move the nozzle up and away from the bed
    do_homing_move(Z_AXIS, Z_CALIB_EXTRA_HIGHT, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z), false, false);
    current_position.z = 0;
    sync_plan_position();
    current_position.x = X_MIN_POS + 2;
    current_position.y = Y_MIN_POS + 2;
    line_to_current_position();
    planner.synchronize();

    const bool z_probe = prusa_toolchanger.is_any_tool_active(); // Use loadcell probe as well as stall if there is a tool picked

    // Home the axis
    endstops.enable(true); // Stall endstops need to be enabled manually as in G28
    if (!homeaxis(Z_AXIS, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z), false, nullptr, true, z_probe)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    endstops.not_homing();

    // Check loadcell before ramming
    if (z_probe) {
        endstops.enable_z_probe(); // Enable z probe to get GetMinZEndstop()
        if (loadcell.GetMinZEndstop()) { // Sitting on the nozzle, cannot ram the Z axis
            fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z); // There was something wrong with the Z homing
        }
        endstops.enable_z_probe(false);
    }

    // Now ram the Z motors into the top position
    static constexpr float target_Z = Z_MIN_POS - Z_CALIB_EXTRA_HIGHT;
    do_blocking_move_to_z(target_Z, Z_CALIB_ALIGN_AXIS_FEEDRATE);

    // Clear some height for the nozzle again
    do_blocking_move_to_z(Z_MIN_POS, Z_CALIB_ALIGN_AXIS_FEEDRATE);

    // Finally mark the axis as unknown
    set_axis_is_not_at_home(Z_AXIS);
}
#else
static constexpr float AFTER_Z_CALIB_Z_POS = 50;

static void safe_move_down() {
    DEPLOY_PROBE();

    // Move to AFTER_Z_CALIB_Z_POS with Z endstop enabled
    float target_Z = AFTER_Z_CALIB_Z_POS - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.z);
    Z_Calib_FSM N(ClientFSM::Selftest, GetPhaseIndex(PhasesSelftest::CalibZ), current_position.z, target_Z, 0, 100);
    if (do_homing_move(AxisEnum::Z_AXIS, target_Z - current_position.z, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z))) {
        // endstop triggered, raise the nozzle
        current_position.z = Z_MIN_POS;
        sync_plan_position();
        move_z_after_probing();
    } else {
        current_position.z = target_Z;
        sync_plan_position();
    }

    STOW_PROBE();
}

void selftest::calib_Z(bool move_down_after) {
    // backup original acceleration/feedrates and reset defaults for calibration
    static constexpr float def_feedrate[] = DEFAULT_MAX_FEEDRATE;
    static constexpr float def_accel[] = DEFAULT_MAX_ACCELERATION;
    float orig_max_feedrate = planner.settings.max_feedrate_mm_s[Z_AXIS];
    float orig_max_accel = planner.settings.max_acceleration_mm_per_s2[Z_AXIS];
    planner.set_max_feedrate(Z_AXIS, def_feedrate[Z_AXIS]);
    planner.set_max_acceleration(Z_AXIS, def_accel[Z_AXIS]);

    // Z axis lift
    marlin_server::fsm_change(PhasesSelftest::CalibZ);
    endstops.enable(true); // Stall endstops need to be enabled manually as in G28
    if (!homeaxis(Z_AXIS, MMM_TO_MMS(HOMING_FEEDRATE_INVERTED_Z), true)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    endstops.not_homing();

    // push both Z axis few mm over HW limit to align motors
    const float target_Z = Z_MAX_POS + Z_CALIB_EXTRA_HIGHT;
    current_position.z = Z_MAX_POS;
    sync_plan_position();
    do_blocking_move_to_z(target_Z, Z_CALIB_ALIGN_AXIS_FEEDRATE);
    current_position.z = Z_MAX_POS;
    sync_plan_position();

    if (move_down_after) {
        safe_move_down();
    }

    // always set axis as unhomed (Z_MAX_POS is unreliable, Z_MIN_POS is not probed with homeaxis()!)
    set_axis_is_not_at_home(Z_AXIS);

    // restore original values
    planner.set_max_feedrate(Z_AXIS, orig_max_feedrate);
    planner.set_max_acceleration(Z_AXIS, orig_max_accel);
}
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 * G162: Z Calibration
 *
 * ## Parameters
 *
 * - `Z` - Calibrate Z axis
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
