// marlin_client.hpp
#pragma once

#include "marlin_client.h"
#include "client_response.hpp"

//-----------------------------------------------------------------------------
// client side functions (can be called from client thread only)

// returns if response send succeeded
// called in client finite state machine
template <class T>
bool marlin_FSM_response(T phase, Response response) {
    uint32_t encoded = ClientResponses::Encode(phase, response);
    if (encoded == uint32_t(-1))
        return false;

    marlin_encoded_response(encoded);
    return true;
}
