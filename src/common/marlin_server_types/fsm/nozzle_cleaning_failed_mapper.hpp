#pragma once

#include <fsm/nozzle_cleaning_failed_phases.hpp>
#include <option/buddy_enable_connect.h>
#include <error_codes.hpp>

constexpr std::optional<ErrCode> nozzle_cleaning_failed_phase_error_code_mapper(const FSMAndPhase nozzle_cleaning_phase) {
    switch (static_cast<PhaseNozzleCleaningFailed>(nozzle_cleaning_phase.phase)) {
    case PhaseNozzleCleaningFailed::cleaning_failed:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED;
#if HAS_NOZZLE_CLEANING_FAILED_PURGING()
    case PhaseNozzleCleaningFailed::recommend_purge:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_RECOMMEND_PURGE;
    case PhaseNozzleCleaningFailed::wait_temp:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_WAIT_TEMP;
    case PhaseNozzleCleaningFailed::purge:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_PURGE;
    case PhaseNozzleCleaningFailed::autoretract:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_AUTORETRACT;
    case PhaseNozzleCleaningFailed::remove_filament:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_REMOVE_FILAMENT;
#endif
#if HAS_SWITCHABLE_AUTO_RETRACT()
    case PhaseNozzleCleaningFailed::offer_auto_retract_enable:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_AUTORETRACT_ENABLE_ASK;
#endif
    case PhaseNozzleCleaningFailed::warn_abort:
        return ErrCode::CONNECT_NOZZLE_CLEANING_FAILED_ABORT_ASK;
    default:
        return std::nullopt;
    }
}
