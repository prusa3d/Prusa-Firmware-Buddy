from enum import Enum
from .marlin_server_types_python_module_impl import *

# enum corresponsing to marlin_server::State
marlin_server_state = Enum("marlin_server_state", marlin_server_state_map)

# enum corresponsign to ClientFSM
fsm_types = Enum("fsm_types", fsm_types_map)

# enum corresponsign to LoadUnloadMode
load_unload_mode = Enum("load_unload_mode", load_unload_mode_map)

# enum corresponsign to PreheatMode
preheat_mode = Enum("preheat_mode", preheat_mode_map)

# enum corresponsign to RetAndCool_t
ret_and_cool = Enum("ret_and_cool", ret_and_cool_map)

# enum corresponsign to WarningType
warning_type = Enum("warning_type", warning_type_map)

phases_load_unload = Enum("phases_load_unload", phases_load_unload_map)
phases_preheat = Enum("phases_preheat", phases_preheat_map)
phases_print_preview = Enum("phases_print_preview", phases_print_preview_map)
phases_selftest = Enum("phases_selftest", phases_selftest_map)
phases_crash_recovery = Enum("phases_crash_recovery",
                             phases_crash_recovery_map)
phases_quick_pause = Enum("phases_quick_pause", phases_quick_pause_map)
phases_warning = Enum("phases_warning", phases_warning_map)
phase_cold_pull = Enum("phase_cold_pull", phase_cold_pull_map)
phases_phase_stepping = Enum("phases_phase_stepping",
                             phases_phase_stepping_map)
phases_printing = Enum("phases_printing", phases_printing_map)
phase_serial_printing = Enum("phase_serial_printing",
                             phase_serial_printing_map)

#FSM responses
fsm_response = Enum("fsm_response", fsm_response_map)


# helper function to fill FSM responses
def fill_responses(phases_enum, responses_data):
    out = {}
    for phase, responses in responses_data.items():
        res = []
        for response in responses:
            res.append(fsm_response(response))
        out[phases_enum(phase)] = res
    return out


# Reponses for each FSM phase
phases_load_unload_responses = fill_responses(
    phases_load_unload, phases_load_unload_responses_data)
phases_preheat_responses = fill_responses(phases_preheat,
                                          phases_preheat_responses_data)
phases_print_preview_responses = fill_responses(
    phases_print_preview, phases_print_preview_responses_data)
phases_crash_recovery_responses = fill_responses(
    phases_crash_recovery, phases_crash_recovery_responses_data)
phases_quick_pause_responses = fill_responses(
    phases_quick_pause, phases_quick_pause_responses_data)
phases_warning_responses = fill_responses(phases_warning,
                                          phases_warning_responses_data)
phases_phase_stepping_responses = fill_responses(
    phases_phase_stepping, phases_phase_stepping_responses_data)
