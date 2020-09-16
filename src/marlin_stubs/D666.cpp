//selftest
#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"

#include "D666.hpp"
#include <stdint.h>
#include "z_calibration_fsm.hpp"

void PrusaGcodeSuite::D666() {
    bool X_test = parser.seen('H');
    bool Y_test = parser.seen('Y');
    bool Z_test = parser.seen('Z');
    bool fan_test = parser.seen('F');
    bool heater_test = parser.seen('H');

    bool axis_test = X_test || Y_test || Z_test;
    bool fan_axis_test = axis_test || fan_test;

    //no parameter? set all tests
    if (!fan_axis_test && heater_test) {
        X_test = Y_test = Z_test = fan_test = heater_test = true;

        axis_test = fan_axis_test = true;
    }

    FSM_Holder D(ClientFSM::SelftestFansAxis, 0);
    while (1)
        ;
    /*    FSM_Holder D(ClientFSM::G162, 0);

    // Z axis lift
    if (parser.seen('Z')) {
        const float target_Z = Z_MAX_POS;
        Z_Calib_FSM N(ClientFSM::G162, GetPhaseIndex(PhasesG162::Parking), current_position.z, target_Z, 0, 100);

        do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }*/
}
