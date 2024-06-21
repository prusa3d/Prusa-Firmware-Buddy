import enum

import marlin_server_types
import pytest


def check_marlin_server_enum(var: enum.Enum):
    assert issubclass(var, enum.Enum)
    print(list(var))


def test_marlin_server_state():
    # print all the members of the enum, try to access some
    check_marlin_server_enum(marlin_server_types.MarlinServerState)
    print(
        f"{marlin_server_types.MarlinServerState.Aborted.name}: {marlin_server_types.MarlinServerState.Aborted.value}"
    )
    print(
        f"{marlin_server_types.MarlinServerState.Finished.name}: {marlin_server_types.MarlinServerState.Finished.value}"
    )


def test_fsm_types():
    # check types of FSM
    check_marlin_server_enum(marlin_server_types.FSMTypes)
    print(
        f"{marlin_server_types.FSMTypes.Preheat.name}: {marlin_server_types.FSMTypes.Preheat.value}"
    )
    print(
        f"{marlin_server_types.FSMTypes.Load_unload.name}: {marlin_server_types.FSMTypes.Load_unload.value}"
    )


def test_other_fsm_types():
    # Helper FSM types
    check_marlin_server_enum(marlin_server_types.LoadUnloadMode)
    check_marlin_server_enum(marlin_server_types.PreheatMode)
    check_marlin_server_enum(marlin_server_types.RetAndCool)
    check_marlin_server_enum(marlin_server_types.WarningType)


def test_fsm_phase_types():
    # Check enums for FSM phases
    check_marlin_server_enum(marlin_server_types.PhasesLoadUnload)
    check_marlin_server_enum(marlin_server_types.PhasesPreheat)
    check_marlin_server_enum(marlin_server_types.PhasesPrintPreview)
    check_marlin_server_enum(marlin_server_types.PhasesSelftest)
    check_marlin_server_enum(marlin_server_types.PhasesCrashRecovery)
    check_marlin_server_enum(marlin_server_types.PhasesQuickPause)
    check_marlin_server_enum(marlin_server_types.PhasesWarning)
    check_marlin_server_enum(marlin_server_types.PhasesColdPull)
    if marlin_server_types.has_phase_stepping:
        check_marlin_server_enum(marlin_server_types.PhasesPhaseStepping)
    check_marlin_server_enum(marlin_server_types.PhasesPrinting)
    check_marlin_server_enum(marlin_server_types.PhasesSerialPrinting)


def test_fsm_responses():
    # Check fsm responses enum
    check_marlin_server_enum(marlin_server_types.FSMResponse)


def test_fsm_phase_responses():
    # test that reponses for each FSM phase exported properly
    print(marlin_server_types.phases_load_unload_responses)
    print(marlin_server_types.phases_load_unload_responses)
    print(marlin_server_types.phases_preheat_responses)
    print(marlin_server_types.phases_print_preview_responses)
    print(marlin_server_types.phases_crash_recovery_responses)
    print(marlin_server_types.phases_quick_pause_responses)
    print(marlin_server_types.phases_warning_responses)
    if hasattr(marlin_server_types, 'phases_phase_stepping_responses'):
        print(marlin_server_types.phases_phase_stepping_responses)


def test_fsm_phase_convert():

    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Serial_printing
    ) == marlin_server_types.PhasesSerialPrinting
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Load_unload
    ) == marlin_server_types.PhasesLoadUnload
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Selftest
    ) == marlin_server_types.PhasesSelftest
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Preheat
    ) == marlin_server_types.PhasesPreheat
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Printing
    ) == marlin_server_types.PhasesPrinting
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.CrashRecovery
    ) == marlin_server_types.PhasesCrashRecovery
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.QuickPause
    ) == marlin_server_types.PhasesQuickPause
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.Warning
    ) == marlin_server_types.PhasesWarning
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.PrintPreview
    ) == marlin_server_types.PhasesPrintPreview
    assert marlin_server_types.fsm_type_to_enum(
        marlin_server_types.FSMTypes.ColdPull
    ) == marlin_server_types.PhasesColdPull
    if hasattr(marlin_server_types, 'phases_phase_stepping'):
        assert marlin_server_types.fsm_type_to_enum(
            marlin_server_types.FSMTypes.PhaseStepping
        ) == marlin_server_types.PhasesPhaseStepping

    with pytest.raises(Exception):
        marlin_server_types.fsm_type_to_enum(None)


def test_selftest_types():
    check_marlin_server_enum(marlin_server_types.ToolMask)
    check_marlin_server_enum(marlin_server_types.SelftestState)


def test_heaters_result():
    data = bytearray(29)
    data[0] = 55  # progress
    data[1] = 0  # heatbreak_error
    # prep_state
    data[2] = marlin_server_types.SelftestSubtestState.ok.value
    # heat_state
    data[3] = marlin_server_types.SelftestSubtestState.not_good.value

    a = marlin_server_types.SelftestHeaters.deserialize(bytes(data))

    nozzle0 = a.noz(0)
    assert (nozzle0.progress == 55)
    assert (nozzle0.heatbreak_error == False)
    assert (nozzle0.prep_state == marlin_server_types.SelftestSubtestState.ok)
    assert (nozzle0.heat_state ==
            marlin_server_types.SelftestSubtestState.not_good)


if __name__ == "__main__":
    test_marlin_server_state()
    test_fsm_types()
    test_fsm_phase_types()
    test_fsm_responses()
    test_fsm_phase_responses()
    if marlin_server_types.has_selftest:
        test_selftest_types()
