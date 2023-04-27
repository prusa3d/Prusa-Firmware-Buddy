#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include <module/endstops.h>

#include "PrusaGcodeSuite.hpp"
#include <stdint.h>
#include "bsod_gui.hpp"
#include "loadcell.h"
#include "z_calibration_fsm.hpp"
#include "calibration_z.hpp"
#include "printers.h"

static constexpr feedRate_t Z_CALIB_ALIGN_AXIS_FEEDRATE = 15.f; // mm/s
static constexpr float Z_CALIB_EXTRA_HIGHT = 5.f;               // mm

#if (PRINTER_TYPE == PRINTER_PRUSA_XL)

    #include <module/prusa/toolchanger.h>

void selftest::calib_Z([[maybe_unused]] bool move_down_after) {
    FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::CalibZ);

    // backup original acceleration/feedrates and reset defaults for calibration
    Temporary_Reset_Motion_Parameters mp;

    // Home XY first
    GcodeSuite::G28_no_parser(false, false, 0, false, true, true, false);

    // Move the nozzle up and away from the bed
    do_homing_move(Z_AXIS, Z_CALIB_EXTRA_HIGHT, HOMING_FEEDRATE_INVERTED_Z, false, false);
    current_position.z = 0;
    sync_plan_position();
    current_position.x = X_MIN_POS + 2;
    current_position.y = Y_MIN_POS + 2;
    line_to_current_position();
    planner.synchronize();

    const bool z_probe = prusa_toolchanger.is_any_tool_active(); // Use loadcell probe as well as stall if there is a tool picked

    // Home the axis
    endstops.enable(true); // Stall endstops need to be enabled manually as in G28
    if (!homeaxis(Z_AXIS, HOMING_FEEDRATE_INVERTED_Z, false, nullptr, true, z_probe)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    endstops.not_homing();

    // Check loadcell before ramming
    if (z_probe) {
        endstops.enable_z_probe();                            // Enable z probe to get GetMinZEndstop()
        if (loadcell.GetMinZEndstop()) {                      // Sitting on the nozzle, cannot ram the Z axis
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
    #if HAS_BED_PROBE
        #define AFTER_Z_CALIB_Z_POS 50

static void safe_move_down() {
    DEPLOY_PROBE();
    float target_Z_offset = (AFTER_Z_CALIB_Z_POS - Z_AFTER_PROBING - Z_MAX_POS);
    current_position.z = Z_MIN_POS;
    Z_Calib_FSM N(ClientFSM::Selftest, GetPhaseIndex(PhasesSelftest::CalibZ), current_position.z, target_Z_offset, 0, 100);
    do_homing_move(AxisEnum::Z_AXIS, target_Z_offset, HOMING_FEEDRATE_INVERTED_Z);
    current_position.z = Z_MIN_POS;
    sync_plan_position();
    STOW_PROBE();
    move_z_after_probing();
}
    #endif //HAS_BED_PROBE

void selftest::calib_Z(bool move_down_after) {
    #if HAS_BED_PROBE
    // backup original acceleration/feedrates and reset defaults for calibration
    static constexpr float def_feedrate[] = DEFAULT_MAX_FEEDRATE;
    static constexpr float def_accel[] = DEFAULT_MAX_ACCELERATION;
    float orig_max_feedrate = planner.settings.max_feedrate_mm_s[Z_AXIS];
    float orig_max_accel = planner.settings.max_acceleration_mm_per_s2[Z_AXIS];
    planner.set_max_feedrate(Z_AXIS, def_feedrate[Z_AXIS]);
    planner.set_max_acceleration(Z_AXIS, def_accel[Z_AXIS]);

    // Z axis lift
    FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::CalibZ);
        #if (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_IXL)
    endstops.enable(true); // Stall endstops need to be enabled manually as in G28
    if (!homeaxis(Z_AXIS, HOMING_FEEDRATE_INVERTED_Z, true)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    endstops.not_homing();
    const float target_Z = Z_MAX_POS + Z_CALIB_EXTRA_HIGHT;
    current_position.z = Z_MAX_POS;
    sync_plan_position();
    do_blocking_move_to_z(target_Z, Z_CALIB_ALIGN_AXIS_FEEDRATE); // push both Z axis few mm over HW limit to align motors
        #endif // (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_IXL)
    if (move_down_after) {
        safe_move_down();
    } else {
        sync_plan_position();
    }

    // restore original values
    planner.set_max_feedrate(Z_AXIS, orig_max_feedrate);
    planner.set_max_acceleration(Z_AXIS, orig_max_accel);
    #endif     //HAS_BED_PROBE
}
#endif

void selftest::ensure_Z_away() {
    static constexpr auto minimum_safe_distance { 10 };

    if (axis_known_position & Z_AXIS) {
        do_blocking_move_to_z(Z_MIN_POS, feedRate_t(Z_CALIB_ALIGN_AXIS_FEEDRATE));
    } else {
        do_blocking_move_to_z(current_position.z + minimum_safe_distance, feedRate_t(Z_CALIB_ALIGN_AXIS_FEEDRATE));
    }
}

void PrusaGcodeSuite::G162() {
    if (parser.seen('Z')) {
        FSM_HOLDER__LOGGING(Selftest);
        selftest::calib_Z(true);
    }
}
