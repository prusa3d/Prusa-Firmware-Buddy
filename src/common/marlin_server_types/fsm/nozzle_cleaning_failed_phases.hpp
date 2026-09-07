/// \file

#pragma once

#include <marlin_server_types/client_response.hpp>
#include <utils/enum_array.hpp>
#include <option/has_nozzle_cleaner.h>
#include <option/has_auto_retract.h>
#include <option/has_switchable_auto_retract.h>
#include <optional>

#define HAS_NOZZLE_CLEANING_FAILED_PURGING() (HAS_AUTO_RETRACT() && !HAS_NOZZLE_CLEANER())

enum class PhaseNozzleCleaningFailed : PhaseUnderlyingType {
    init,

    // Nozzle cleaning failed warning
    cleaning_failed,

#if HAS_NOZZLE_CLEANING_FAILED_PURGING()
    // On retry
    recommend_purge,

    // Wait for temp
    wait_temp,

    // Purge the filament
    purge,

    // Autoretract
    autoretract,

    // After purge
    remove_filament,
#endif

#if HAS_SWITCHABLE_AUTO_RETRACT()
    /// Offer the user to enable auto-retract to prevent failures in the future
    offer_auto_retract_enable,
#endif

    // warn user what abort really does
    warn_abort,

    _cnt,
    _last = _cnt - 1
};

struct NozzleCleaningFailedProgressData {
    /// 0-255
    uint8_t progress_0_255 = 0;
};

namespace ClientResponses {

inline constexpr EnumArray<PhaseNozzleCleaningFailed, PhaseResponses, PhaseNozzleCleaningFailed::_cnt> nozzle_cleaning_responses {
    { PhaseNozzleCleaningFailed::init, {} },
        { PhaseNozzleCleaningFailed::cleaning_failed, { Response::Retry, Response::Ignore, Response::Abort } },
#if HAS_NOZZLE_CLEANING_FAILED_PURGING()
        { PhaseNozzleCleaningFailed::recommend_purge, { Response::Yes, Response::No } },
        { PhaseNozzleCleaningFailed::wait_temp, { Response::Abort } },
        { PhaseNozzleCleaningFailed::purge, { Response::Abort } },
        { PhaseNozzleCleaningFailed::autoretract, {} },
        { PhaseNozzleCleaningFailed::remove_filament, { Response::Done, Response::Abort } },
#endif
#if HAS_SWITCHABLE_AUTO_RETRACT()
        { PhaseNozzleCleaningFailed::offer_auto_retract_enable, { Response::Yes, Response::No } },
#endif
        { PhaseNozzleCleaningFailed::warn_abort, { Response::Yes, Response::No } },
};

} // namespace ClientResponses

constexpr inline ClientFSM client_fsm_from_phase(PhaseNozzleCleaningFailed) { return ClientFSM::NozzleCleaningFailed; }
