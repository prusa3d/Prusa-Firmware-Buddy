/// \file

#pragma once

#include <fsm/wastebin_emptying_phases.hpp>
#include <error_codes.hpp>

constexpr std::optional<ErrCode> wastebin_emptying_phase_error_code_mapper(const FSMAndPhase wastebin_emptying_phase) {
    switch (static_cast<PhaseWastebinEmptying>(wastebin_emptying_phase.phase)) {
    case PhaseWastebinEmptying::prompt_full:
        return ErrCode::CONNECT_NOZZLE_CLEANER_FULL;
    case PhaseWastebinEmptying::prompt_empty:
        return ErrCode::CONNECT_NOZZLE_CLEANER_EMPTY;
    default:
        return std::nullopt;
    }
}
