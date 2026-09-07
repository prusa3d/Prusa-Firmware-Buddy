/// @file
#pragma once

#include <marlin_server_types/client_response.hpp>

enum class PhaseGearboxAlignment : PhaseUnderlyingType {
    intro,
    filament_loaded_ask_unload,
    filament_unknown_ask_unload,
    loosen_screws,
    alignment,
    tighten_screws,
    done,
    finish,
    _last = finish,
};

constexpr inline ClientFSM client_fsm_from_phase(PhaseGearboxAlignment) { return ClientFSM::GearboxAlignment; }

namespace ClientResponses {
inline constexpr EnumArray<PhaseGearboxAlignment, PhaseResponses, CountPhases<PhaseGearboxAlignment>()> gearbox_alignment_responses {
    { PhaseGearboxAlignment::intro, { Response::Continue, Response::Skip } },
    { PhaseGearboxAlignment::filament_loaded_ask_unload, { Response::Unload, Response::Abort } },
    { PhaseGearboxAlignment::filament_unknown_ask_unload, { Response::Continue, Response::Unload, Response::Abort } },
    { PhaseGearboxAlignment::loosen_screws, { Response::Continue, Response::Skip } },
    { PhaseGearboxAlignment::alignment, {} },
    { PhaseGearboxAlignment::tighten_screws, { Response::Continue } },
    { PhaseGearboxAlignment::done, { Response::Continue } },
    { PhaseGearboxAlignment::finish, {} },
};
}

/// Data struct passed between the FSM and GUI
struct FSMGearboxAlignmentData {
    uint8_t physical_tool_index;
};
