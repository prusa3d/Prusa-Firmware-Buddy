/// @file
#pragma once

#include <marlin_server_types/client_response.hpp>
#include <utils/enum_array.hpp>

struct SquarenessData {
    int16_t dy_um = 0; ///< Gantry skew (right minus left dock stop) [um]
    bool valid = false; ///< The measurement succeeded
};

enum class PhaseGantrySquareness : PhaseUnderlyingType {
    /// Introduction
    intro,

    /// Waiting while the printer parks the picked tool and homes
    preparing,

    /// Ask the user to empty the outermost docks
    remove_nozzles,

    /// Waiting while the printer measures the skew against the docks
    measuring,

    /// The measured skew is within the limit
    result_ok,

    /// The measured skew exceeds the limit, or the measurement failed
    result_failed,

    /// Ask the user to put the nozzles back into the outermost docks
    reinsert_nozzles,

    _cnt,
    _last = _cnt - 1
};

namespace ClientResponses {
extern constinit const EnumArray<PhaseGantrySquareness, PhaseResponses, PhaseGantrySquareness::_cnt> gantry_squareness_responses;
} // namespace ClientResponses

constexpr inline ClientFSM client_fsm_from_phase(PhaseGantrySquareness) { return ClientFSM::GantrySquareness; }
