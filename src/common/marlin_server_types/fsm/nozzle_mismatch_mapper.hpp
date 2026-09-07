/// \file

#pragma once

#include <fsm/nozzle_mismatch_phases.hpp>
#include <error_codes.hpp>

constexpr std::optional<ErrCode> nozzle_mismatch_phase_error_code_mapper(const FSMAndPhase nozzle_mismatch_phase) {
    switch (static_cast<PhaseNozzleMismatch>(nozzle_mismatch_phase.phase)) {
    case PhaseNozzleMismatch::prompt:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_UNKNOWN;
    case PhaseNozzleMismatch::dock_not_empty:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_DOCK_NOT_EMPTY;
    case PhaseNozzleMismatch::tool_lost:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_TOOL_LOST;
    case PhaseNozzleMismatch::pickup_failed:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_PICKUP_FAILED;
    case PhaseNozzleMismatch::park_failed:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_PARK_FAILED;
    case PhaseNozzleMismatch::confirm_abort:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_CONFIRM_ABORT;
    default:
        return std::nullopt;
    }
}
