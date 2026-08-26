/// \file

#pragma once

#include <marlin_server_types/client_response.hpp>
#include <utils/enum_array.hpp>
#include <utils/storage/enum_bitset.hpp>

enum class PhaseWastebinEmptying : PhaseUnderlyingType {
    parking,
    prompt_full,
    prompt_empty,
    raising_bed,
    heating,
    purging,
    returning,

    _cnt,
    _last = _cnt - 1
};

namespace ClientResponses {
extern constinit const EnumArray<PhaseWastebinEmptying, PhaseResponses, PhaseWastebinEmptying::_cnt> wastebin_emptying_responses;
} // namespace ClientResponses

extern constinit const EnumBitset<PhaseWastebinEmptying, PhaseWastebinEmptying::_cnt> wastebin_emptying_is_phase_attention;

constexpr inline ClientFSM client_fsm_from_phase(PhaseWastebinEmptying) { return ClientFSM::WastebinEmptying; }
