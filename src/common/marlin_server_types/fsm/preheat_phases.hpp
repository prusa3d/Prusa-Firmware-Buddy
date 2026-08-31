/// @file
#pragma once

#include <marlin_server_types/client_response.hpp>

#include <option/has_anfc.h>

enum class PhasesPreheat : PhaseUnderlyingType {
    user_temp_selection,

#if HAS_ANFC()
    /// Wait screen while communicating with the OPT
    loading_openprinttag,

    /// Asks the user to load the data from OpenPrintTag
    ask_load_openprinttag,

    /// Shows a screen with filament parameters loaded from the OpenPrintTag,
    /// allowing the user to adjust them
    /// This is typically necessary when the parameters on the tag are incomplete.
    /// !!! Currently, this is implemented as a hack where the GUI opens ScreenOPTFilamentDetail and immediately sends Response::Ok
    /// !!! This then switches the FSM to user_temp_selection; the OPT is handled in a standard PendingAdHocFilament way
    openprinttag_parameters,
#endif

    _cnt,
    _last = _cnt - 1
};

namespace ClientResponses {

inline constexpr EnumArray<PhasesPreheat, PhaseResponses, PhasesPreheat::_cnt> preheat_responses {
    // Additionally, filament type selection is passed through FSMResponseVariant(FilamentType)
    { PhasesPreheat::user_temp_selection, { Response::Abort, Response::Cooldown } },

#if HAS_ANFC()
        { PhasesPreheat::loading_openprinttag, {} },
        { PhasesPreheat::ask_load_openprinttag, { Response::Yes, Response::No } },
        { PhasesPreheat::openprinttag_parameters, {} },
#endif
};

} // namespace ClientResponses

constexpr inline ClientFSM client_fsm_from_phase(PhasesPreheat) { return ClientFSM::Preheat; }
