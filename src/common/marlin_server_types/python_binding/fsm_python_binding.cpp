#include "marlin_server_state.h"
#include "client_fsm_types.h"
#include "client_response_texts.hpp"
#include "client_response.hpp"
#include "magic_enum.hpp"
#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using namespace nb::literals;

template <typename enum_T>
void export_enum_map(::nanobind::module_ &m, char const *name) {
    std::map<std::string, int> enum_map;
    for (const auto &entry : magic_enum::enum_entries<enum_T>()) {
        enum_map.insert({ entry.second.begin(), static_cast<int>(entry.first) });
    }
    m.attr(name) = enum_map;
}

template <typename enum_T>
void export_phase_responses(::nanobind::module_ &m, char const *name) {
    std::map<int, std::vector<int>> responses_map;
    for (const auto &entry : magic_enum::enum_entries<enum_T>()) {
        std::vector<int> responses;
        for (const auto response : ClientResponses::GetResponses(entry.first)) {
            if (static_cast<int>(response) != 0) {
                responses.push_back(static_cast<int>(response));
            }
        }
        responses_map.insert({ static_cast<int>(entry.first), responses });
    }

    m.attr(name) = responses_map;
}

NB_MODULE(marlin_server_types_python_module_impl, m) {
    m.doc() = "Python library exposing types used in marlin server and FSM";

    // Export basic enums from marlin_server_state.h
    export_enum_map<marlin_server::State>(m, "marlin_server_state_map");

    // Export basic enums from client_fsm_types.h
    export_enum_map<ClientFSM>(m, "fsm_types_map");
    export_enum_map<LoadUnloadMode>(m, "load_unload_mode_map");
    export_enum_map<PreheatMode>(m, "preheat_mode_map");
    export_enum_map<RetAndCool_t>(m, "ret_and_cool_map");
    export_enum_map<WarningType>(m, "warning_type_map");

    // export basic enums from client_response.hpp
    export_enum_map<PhasesLoadUnload>(m, "phases_load_unload_map");
    export_phase_responses<PhasesLoadUnload>(m, "phases_load_unload_responses_data");
    export_enum_map<PhasesPreheat>(m, "phases_preheat_map");
    export_phase_responses<PhasesPreheat>(m, "phases_preheat_responses_data");
    export_enum_map<PhasesPrintPreview>(m, "phases_print_preview_map");
    export_phase_responses<PhasesPrintPreview>(m, "phases_print_preview_responses_data");
    export_enum_map<PhasesSelftest>(m, "phases_selftest_map");
    export_enum_map<PhasesCrashRecovery>(m, "phases_crash_recovery_map");
    export_phase_responses<PhasesCrashRecovery>(m, "phases_crash_recovery_responses_data");
    export_enum_map<PhasesQuickPause>(m, "phases_quick_pause_map");
    export_phase_responses<PhasesQuickPause>(m, "phases_quick_pause_responses_data");
    export_enum_map<PhasesWarning>(m, "phases_warning_map");
    export_phase_responses<PhasesWarning>(m, "phases_warning_responses_data");
    export_enum_map<PhasesColdPull>(m, "phase_cold_pull_map");
    export_enum_map<PhasesPhaseStepping>(m, "phases_phase_stepping_map");
    export_phase_responses<PhasesPhaseStepping>(m, "phases_phase_stepping_responses_data");
    export_enum_map<PhasesPrinting>(m, "phases_printing_map");
    export_enum_map<PhasesSerialPrinting>(m, "phase_serial_printing_map");

    // export FSM response
    export_enum_map<Response>(m, "fsm_response_map");
}
