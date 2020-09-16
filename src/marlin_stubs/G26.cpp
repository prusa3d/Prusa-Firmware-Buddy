#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "client_fsm_types.h"

void GcodeSuite::G26() {
    fsm_create(ClientFSM::FirstLayer);

    do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));

    fsm_destroy(ClientFSM::FirstLayer);
}

#endif // HOST_PROMPT_SUPPORT && !EMERGENCY_PARSER
