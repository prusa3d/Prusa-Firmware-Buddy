#pragma once

#include <client_response.hpp>
#include <common/primitive_any.hpp>

using FSMResponseVariant = PrimitiveAny<4>;

// This holds type-erased FSM response to reduce module dependencies
struct EncodedFSMResponse {
    FSMResponseVariant response;
    FSMAndPhase fsm_and_phase;
};

#ifndef UNITTESTS
static_assert(sizeof(EncodedFSMResponse) <= 12);
#endif
