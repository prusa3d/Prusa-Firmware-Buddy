// \file
#pragma once

#include "client_fsm_types.hpp"

/// FSM data structure for MOST states of ClientFSM::Load_unload
/// !!! Not for all states, for example MMU_ERRWaitingForUser uses a different one
struct FSMLoadUnloadData {
    LoadUnloadMode mode;
    uint8_t progress;
};
