/// \file

#pragma once

#include <marlin_server_types/client_response.hpp>
#include <utils/enum_array.hpp>

enum class PhaseNozzleMismatch : PhaseUnderlyingType {
    /// Prompt informing the user a nozzle was detected but its identity is unknown
    prompt,

    /// Scrollable menu for selecting which dock the tool belongs to
    dock_selection,

    /// Waiting while the printer bumps to the selected dock
    parking,

    /// Warning: head hit something on the way to the dock
    dock_not_empty,

    /// Tool expected by EEPROM but not detected
    tool_lost,

    /// Waiting while homing after tool loss
    homing,

    /// Nozzle not detected after tool pickup
    pickup_failed,

    /// Nozzle still detected after parking
    park_failed,

    /// Abort confirmation after pickup/park failure
    confirm_abort,

    _cnt,
    _last = _cnt - 1
};

namespace ClientResponses {
extern constinit const EnumArray<PhaseNozzleMismatch, PhaseResponses, PhaseNozzleMismatch::_cnt> nozzle_mismatch_responses;
} // namespace ClientResponses

constexpr inline ClientFSM client_fsm_from_phase(PhaseNozzleMismatch) { return ClientFSM::NozzleMismatch; }
