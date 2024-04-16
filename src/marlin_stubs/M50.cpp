/**
 * @file
 * @brief selftest
 */
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"

#include "M50.hpp"
#include <stdint.h>
#include "z_calibration_fsm.hpp"
#include <guiconfig/wizard_config.hpp>

#include <option/has_gui.h>

#if HAS_GUI()

/** \addtogroup G-Codes
 * @{
 */

/**
 * @brief selftest
 *
 * use M50 because M48 is test of Z probing (also some kind of test)
 * and M49 was used
 */
void PrusaGcodeSuite::M50() {
    bool X_test = parser.seen('X');
    bool Y_test = parser.seen('Y');
    bool Z_test = parser.seen('Z');
    bool fan_test = parser.seen('F');
    bool heater_test = parser.seen('H');

    bool axis_test = X_test || Y_test || Z_test;
    bool fan_axis_test = axis_test || fan_test;

    // no parameter? set all tests
    if (!fan_axis_test && heater_test) {
        X_test = Y_test = Z_test = fan_test = heater_test = true;

        axis_test = fan_axis_test = true;
    }

    {
        FSM_HOLDER__LOGGING(SelftestAxis);
        const float target_Z = 20;
        Z_Calib_FSM N(ClientFSM::SelftestAxis, GetPhaseIndex(PhasesSelftest::CalibZ), current_position.z, target_Z, 0, 100); // bind to variable and automatically notify progress
        do_blocking_move_to_z(20, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }
    /*    FSM_HOLDER__LOGGING(G162);

    // Z axis lift
    if (parser.seen('Z')) {
        const float target_Z = Z_MAX_POS;
        Z_Calib_FSM N(ClientFSM::G162, GetPhaseIndex(PhasesG162::Parking), current_position.z, target_Z, 0, 100);

        do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }*/
}

/** @}*/

#else

void PrusaGcodeSuite::M50() {}

#endif
