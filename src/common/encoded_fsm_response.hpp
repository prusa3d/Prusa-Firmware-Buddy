#pragma once

// This holds type-erased FSM response to reduce module dependencies
struct EncodedFSMResponse {
    uint8_t encoded_phase;
    uint8_t encoded_fsm;
    uint8_t encoded_response;
};
static_assert(sizeof(EncodedFSMResponse) <= 4);
