// marlin_client.hpp
#pragma once

#include "marlin_client.h"
#include "dialog_commands.hpp"

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// click button, return success
template <class T>
bool marlin_FSM_response(T phase, Response command) {
    uint32_t encoded = ClientResponses::Encode(phase, command);
    if (encoded == uint32_t(-1))
        return false;

    marlin_encoded_response(encoded);
    return true;
}
