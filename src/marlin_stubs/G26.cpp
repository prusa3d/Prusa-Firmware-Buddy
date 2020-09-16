#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/feature/host_actions.h"
#include "../../lib/Marlin/Marlin/src/feature/safety_timer.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../lib/Marlin/Marlin/src/Marlin.h"
#include "marlin_server.hpp"
#include "client_fsm_types.h"
#include "PrusaGcodeSuite.hpp"

void PrusaGcodeSuite::G26() {
    fsm_create(ClientFSM::FirstLayer);

    //do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));

    fsm_destroy(ClientFSM::FirstLayer);
}
