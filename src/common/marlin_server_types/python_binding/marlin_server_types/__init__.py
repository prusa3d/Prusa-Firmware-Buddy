from enum import Enum
from typing import Dict, List, TypeVar

from .marlin_server_types_python_module_impl import (
    FSMResponse,
    FSMTypes,
    LoadUnloadMode,
    MarlinServerState,
    PhasesColdPull,
    PhasesCrashRecovery,
    PhasesLoadUnload,
    PhasesPreheat,
    PhasesPrintPreview,
    PhasesPrinting,
    PhasesQuickPause,
    PhasesSelftest,
    PhasesSerialPrinting,
    PhasesWarning,
    PreheatMode,
    RetAndCool,
    WarningType,
)
from .marlin_server_types_python_module_impl import (
    phases_crash_recovery_responses_data,
    phases_load_unload_responses_data,
    phases_preheat_responses_data,
    phases_print_preview_responses_data,
    phases_quick_pause_responses_data,
    phases_warning_responses_data,
)

# depending on python version used, import Never from typing or typing_extensions
try:
    from typing import Never
except ImportError:
    from typing_extensions import Never

__all__ = [
    "FSMResponse",
    "FSMTypes",
    "LoadUnloadMode",
    "MarlinServerState",
    "PhasesColdPull",
    "PhasesCrashRecovery",
    "PhasesLoadUnload",
    "PhasesPreheat",
    "PhasesPrintPreview",
    "PhasesPrinting",
    "PhasesQuickPause",
    "PhasesSelftest",
    "PhasesSerialPrinting",
    "PhasesWarning",
    "PreheatMode",
    "RetAndCool",
    "WarningType",
    "PhasesEnumTypes",
    "fsm_type_to_enum",
]

T = TypeVar("T", bound=Enum)


def build_responses(phases_enum: T,
                    responses_data) -> Dict[T, List[FSMResponse]]:
    """
    Build dictionary of responses for each phase in the enum.
    """
    out = {}
    for phase, responses in responses_data.items():
        res = []
        for response in responses:
            res.append(FSMResponse(response))
        out[phases_enum(phase)] = res  # type: ignore
    return out


# Reponses for each FSM phase
phases_load_unload_responses = build_responses(
    PhasesLoadUnload, phases_load_unload_responses_data)
phases_preheat_responses = build_responses(PhasesPreheat,
                                           phases_preheat_responses_data)
phases_print_preview_responses = build_responses(
    PhasesPrintPreview, phases_print_preview_responses_data)
phases_crash_recovery_responses = build_responses(
    PhasesCrashRecovery, phases_crash_recovery_responses_data)
phases_quick_pause_responses = build_responses(
    PhasesQuickPause, phases_quick_pause_responses_data)
phases_warning_responses = build_responses(PhasesWarning,
                                           phases_warning_responses_data)

__all__ += [
    "phases_load_unload_responses", "phases_preheat_responses",
    "phases_print_preview_responses", "phases_crash_recovery_responses",
    "phases_quick_pause_responses", "phases_warning_responses",
    "has_phase_stepping"
]

# Type definition for any FSM phase enum
PhasesEnumTypes = PhasesLoadUnload | PhasesPreheat | PhasesPrintPreview | PhasesSelftest | PhasesCrashRecovery | PhasesQuickPause | PhasesWarning | PhasesColdPull | PhasesPrinting | PhasesSerialPrinting

try:
    # phase stepping is not always present

    from .marlin_server_types_python_module_impl import (
        PhasesPhaseStepping,
        phases_phase_stepping_responses_data,
    )
    phases_phase_stepping_responses = build_responses(
        PhasesPhaseStepping, phases_phase_stepping_responses_data)

    has_phase_stepping = True

    __all__ += ["PhasesPhaseStepping", "phases_phase_stepping_responses"]

    PhasesEnumTypes |= PhasesPhaseStepping

except ImportError:
    has_phase_stepping = False
    pass

try:
    # selftest types are not allways preset

    from .marlin_server_types_python_module_impl import (
        ToolMask,
        SelftestState,
        SelftestSubtestState,
        SelftestHeater,
        SelftestHeaters,
    )

    has_selftest_types = True

    __all__ += [
        "ToolMask", "SelftestState", "SelftestSubtestState", "SelftestHeater",
        "SelftestHeaters"
    ]

except ImportError:
    has_selftest_types = False


def unknown_fsm_type(fsm_type: FSMTypes) -> Never:
    raise AssertionError("FSM type not recognized: " + str(fsm_type))


def fsm_type_to_enum(fsm_type: FSMTypes) -> type[PhasesEnumTypes]:
    """ For specified fsm_type, return Enum with phases of that FSM. """

    match fsm_type:
        case FSMTypes.CrashRecovery:
            return PhasesCrashRecovery
        case FSMTypes.ColdPull:
            return PhasesColdPull
        case FSMTypes.Serial_printing:
            return PhasesSerialPrinting
        case FSMTypes.Load_unload:
            return PhasesLoadUnload
        case FSMTypes.Preheat:
            return PhasesPreheat
        case FSMTypes.Selftest:
            return PhasesSelftest
        case FSMTypes.Printing:
            return PhasesPrinting
        case FSMTypes.QuickPause:
            return PhasesQuickPause
        case FSMTypes.QuickPause:
            return PhasesQuickPause
        case FSMTypes.Warning:
            return PhasesWarning
        case FSMTypes.PrintPreview:
            return PhasesPrintPreview
        case _:
            if (has_phase_stepping):
                if fsm_type == FSMTypes.PhaseStepping:
                    return PhasesPhaseStepping  # type: ignore
            return unknown_fsm_type(fsm_type)
