#include "marlin_server_state.h"
#include "client_fsm_types.hpp"
#include "client_response.hpp"
#include <warning_type.hpp>
#include "magic_enum.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/array.h>

#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include "selftest_types.hpp"
    #include "printer_selftest.hpp"
    #include "selftest/selftest_sub_state.hpp"
    #include "selftest_heaters_type.hpp"
#endif
#include "fsm/filament_change_phases.hpp"
#include <fsm/print_preview_phases.hpp>
#include <fsm/preheat_phases.hpp>

#include <bsod/bsod.h>

namespace nb = nanobind;
using namespace nb::literals;

template <typename enum_T>
void export_enum(::nanobind::module_ &m, char const *name) {
    auto enumeration = ::nanobind::enum_<enum_T>(m, name, ::nanobind::is_arithmetic());
    for (const auto &entry : magic_enum::enum_entries<enum_T>()) {
        enumeration.value(entry.second.begin(), entry.first);
    }
}

template <typename enum_T>
void export_phase_responses(::nanobind::module_ &m, char const *name) {
    std::map<int, std::vector<int>> responses_map;
    for (const auto &entry : magic_enum::enum_entries<enum_T>()) {
        std::vector<int> responses;
        for (const auto response : ClientResponses::get_available_responses(entry.first)) {
            if (static_cast<int>(response) != 0) {
                responses.push_back(static_cast<int>(response));
            }
        }
        responses_map.insert({ static_cast<int>(entry.first), responses });
    }

    m.attr(name) = responses_map;
}

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    (void)file_name, (void)line_number;
    throw std::runtime_error(fmt);
}

#if HAS_SELFTEST()
SelftestHeaters_t deserialize(nb::bytes &data) {
    if (data.size() != sizeof(SelftestHeaters_t)) {
        throw std::runtime_error("Invalid data size");
    }
    SelftestHeaters_t result;
    memcpy(&result, data.data(), sizeof(SelftestHeaters_t));
    return result;
}

void export_selftest_heaters_result(::nanobind::module_ &m) {

    auto selftest_heater = nb::class_<SelftestHeater_t>(m, "SelftestHeater")
                               .def(nb::init<>())
                               .def_rw("progress", &SelftestHeater_t::progress)
                               .def_rw("heatbreak_error", &SelftestHeater_t::heatbreak_error)
                               .def_rw("prep_state", &SelftestHeater_t::prep_state)
                               .def_rw("heat_state", &SelftestHeater_t::heat_state);

    auto selftest_heaters = nb::class_<SelftestHeaters_t>(m, "SelftestHeaters")
                                .def(nb::init<>())
                                .def_static("deserialize", &deserialize)
                                .def("noz", [](const SelftestHeaters_t &selftest_heaters, int index) -> SelftestHeater_t {
                                    return selftest_heaters.noz[PhysicalToolIndex::from_raw(index)];
                                })
                                .def_rw("bed", &SelftestHeaters_t::bed);
}
#endif

NB_MODULE(marlin_server_types_python_module_impl, m) {
    m.doc() = "Python library exposing types used in marlin server and FSM";

    // Export basic enums from marlin_server_state.h
    export_enum<marlin_server::State>(m, "MarlinServerState");

    // Export basic enums from client_fsm_types.hpp
    export_enum<ClientFSM>(m, "FSMTypes");
    export_enum<LoadUnloadMode>(m, "LoadUnloadMode");
    export_enum<PreheatMode>(m, "PreheatMode");
    export_enum<RetAndCool_t>(m, "RetAndCool");
    export_enum<WarningType>(m, "WarningType");

#if HAS_SELFTEST()
    // basic enums from selftest_types.hpp
    export_enum<SelftestState_t>(m, "SelftestState");
    export_enum<SelftestSubtestState_t>(m, "SelftestSubtestState");

    export_selftest_heaters_result(m);
#endif

    // export basic enums from client_response.hpp
    export_enum<PhasesLoadUnload>(m, "PhasesLoadUnload");
    export_phase_responses<PhasesLoadUnload>(m, "phases_load_unload_responses_data");

    export_enum<PhasesPreheat>(m, "PhasesPreheat");
    export_phase_responses<PhasesPreheat>(m, "phases_preheat_responses_data");

    export_enum<PhasesPrintPreview>(m, "PhasesPrintPreview");
    export_phase_responses<PhasesPrintPreview>(m, "phases_print_preview_responses_data");

#if HAS_SELFTEST()
    export_enum<PhasesSelftest>(m, "PhasesSelftest");
    export_phase_responses<PhasesSelftest>(m, "phases_selftest_responses_data");
#endif

    export_enum<PhasesCrashRecovery>(m, "PhasesCrashRecovery");
    export_phase_responses<PhasesCrashRecovery>(m, "phases_crash_recovery_responses_data");

    export_enum<PhasesQuickPause>(m, "PhasesQuickPause");
    export_phase_responses<PhasesQuickPause>(m, "phases_quick_pause_responses_data");

    export_enum<PhasesWarning>(m, "PhasesWarning");
    export_phase_responses<PhasesWarning>(m, "phases_warning_responses_data");

    export_enum<PhasesColdPull>(m, "PhasesColdPull");

    export_enum<PhasesPrinting>(m, "PhasesPrinting");

#if HAS_SERIAL_PRINT()
    export_enum<PhasesSerialPrinting>(m, "PhasesSerialPrinting");
#endif

    export_enum<Response>(m, "FSMResponse");

#if HAS_PHASE_STEPPING_CALIBRATION()
    export_enum<PhasesPhaseStepping>(m, "PhasesPhaseStepping");
    export_phase_responses<PhasesPhaseStepping>(m, "phases_phase_stepping_responses_data");
#endif
}
