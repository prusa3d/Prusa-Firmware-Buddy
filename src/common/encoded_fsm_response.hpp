#pragma once

#include <client_response.hpp>
#include <common/primitive_any.hpp>

using FSMResponseVariant = PrimitiveAny<4>;

// This holds type-erased FSM response to reduce module dependencies
struct EncodedFSMResponse {

    template <typename Phase>
    static EncodedFSMResponse encode(Phase phase, FSMResponseVariant response) {
        return EncodedFSMResponse {
            .response = response,
            .encoded_phase = ftrstd::to_underlying(phase),
            .encoded_fsm = ftrstd::to_underlying(client_fsm_from_phase(phase)),
        };
    }

    FSMResponseVariant response;
    uint8_t encoded_phase;
    uint8_t encoded_fsm;
};

#ifndef UNITTESTS
static_assert(sizeof(EncodedFSMResponse) <= 12);
#endif
