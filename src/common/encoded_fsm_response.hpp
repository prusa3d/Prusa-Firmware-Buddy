#pragma once

#include <client_response.hpp>

// This holds type-erased FSM response to reduce module dependencies
struct EncodedFSMResponse {

    template <typename Phase>
    static EncodedFSMResponse encode(Phase phase, Response response) {
        return EncodedFSMResponse {
            .encoded_phase = ftrstd::to_underlying(phase),
            .encoded_fsm = ftrstd::to_underlying(client_fsm_from_phase(phase)),
            .encoded_response = ftrstd::to_underlying(response),
        };
    }

    uint8_t encoded_phase;
    uint8_t encoded_fsm;
    uint8_t encoded_response;
};
static_assert(sizeof(EncodedFSMResponse) <= 4);
