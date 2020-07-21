#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"

#include "G162.hpp"
#include <stdint.h>
#include "z_calibration_fsm.hpp"

void PrusaGcodeSuite::G162() {
    FSM_Holder D(ClientFSM::G162, 0);

    // Z axis lift
    if (parser.seen('Z')) {
        const float target_Z = Z_MAX_POS;
        Z_Calib_FSM N(ClientFSM::G162, GetPhaseIndex(PhasesG162::Parking), current_position.z, target_Z, 0, 100);

        do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }
}
