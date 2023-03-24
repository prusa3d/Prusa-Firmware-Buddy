#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"

#include "PrusaGcodeSuite.hpp"
#include <stdint.h>
#include "z_calibration_fsm.hpp"
#include "calibration_z.hpp"
#include "printers.h"

#define Z_CALIB_ALIGN_AXIS_FEEDRATE 15

#define AFTER_Z_CALIB_Z_POS 50
#define Z_CALIB_EXTRA_HIGHT 5

#if HAS_BED_PROBE
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
    homeaxis(Z_AXIS, HOMING_FEEDRATE_INVERTED_Z, true);
    #if (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_IXL || PRINTER_TYPE == PRINTER_PRUSA_XL)
    const float target_Z = Z_MAX_POS + Z_CALIB_EXTRA_HIGHT;
    current_position.z = Z_MAX_POS;
    sync_plan_position();
    do_blocking_move_to_z(target_Z, feedRate_t(Z_CALIB_ALIGN_AXIS_FEEDRATE)); // push both Z axis few mm over HW limit to align motors
    #endif                                                                    // (PRINTER_TYPE == PRINTER_PRUSA_MK4 || PRINTER_TYPE == PRINTER_PRUSA_IXL || PRINTER_TYPE == PRINTER_PRUSA_XL)
    if (move_down_after) {
        safe_move_down();
    } else {
        sync_plan_position();
    }

    // restore original values
    planner.set_max_feedrate(Z_AXIS, orig_max_feedrate);
    planner.set_max_acceleration(Z_AXIS, orig_max_accel);
#endif //HAS_BED_PROBE
}

void PrusaGcodeSuite::G162() {
    if (parser.seen('Z')) {
        FSM_HOLDER__LOGGING(Selftest, 0);
        selftest::calib_Z(true);
    }
}
