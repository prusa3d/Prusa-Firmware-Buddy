#pragma once

#include <common/fsm_base_types.hpp>
#include "client_response.hpp"
#include "feature/phase_stepping/calibration.hpp"
#include "selftest_sub_state.hpp"

struct SelftestPhaseSteppingResult {
    phase_stepping::CalibrationResult result_x { phase_stepping::CalibrationResult::make_unknown() };
    phase_stepping::CalibrationResult result_y { phase_stepping::CalibrationResult::make_unknown() };

    constexpr SelftestPhaseSteppingResult() {}

    fsm::PhaseData serialize(PhasesSelftest) const;

    constexpr bool operator==([[maybe_unused]] const SelftestPhaseSteppingResult &other) const {
        return true;
    }

    constexpr bool operator!=(const SelftestPhaseSteppingResult &other) const {
        return !((*this) == other);
    }

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};
