#pragma once

#include "selftest_phase_stepping_result.hpp"
#include <client_response.hpp>

namespace selftest {

struct SelftestPhaseSteppingConfig {
    using type_evaluation = SelftestPhaseSteppingResult;
    static constexpr SelftestParts part_type = SelftestParts::PhaseStepping;
};

}; // namespace selftest
