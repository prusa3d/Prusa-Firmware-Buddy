import marlin_server_types
import enum


def check_marlin_server_enum(var: enum.Enum):
    assert issubclass(var, enum.Enum)
    assert var.__module__ == "marlin_server_types"
    print(list(var))


def test_marlin_server_state():
    # print all the members of the enum, try to access some
    check_marlin_server_enum(marlin_server_types.marlin_server_state)
    print(
        f"{marlin_server_types.marlin_server_state.Aborted.name}: {marlin_server_types.marlin_server_state.Aborted.value}"
    )
    print(
        f"{marlin_server_types.marlin_server_state.Finished.name}: {marlin_server_types.marlin_server_state.Finished.value}"
    )


def test_fsm_types():
    # check types of FSM
    check_marlin_server_enum(marlin_server_types.fsm_types)
    print(
        f"{marlin_server_types.fsm_types.Preheat.name}: {marlin_server_types.fsm_types.Preheat.value}"
    )
    print(
        f"{marlin_server_types.fsm_types.Load_unload.name}: {marlin_server_types.fsm_types.Load_unload.value}"
    )


def test_other_fsm_types():
    # Helper FSM types
    check_marlin_server_enum(marlin_server_types.load_unload_mode)
    check_marlin_server_enum(marlin_server_types.preheat_mode)
    check_marlin_server_enum(marlin_server_types.ret_and_cool)
    check_marlin_server_enum(marlin_server_types.warning_type)


def test_fsm_phase_types():
    # Check enums for FSM phases
    check_marlin_server_enum(marlin_server_types.phases_load_unload)
    check_marlin_server_enum(marlin_server_types.phases_load_unload)
    check_marlin_server_enum(marlin_server_types.phases_preheat)
    check_marlin_server_enum(marlin_server_types.phases_print_preview)
    check_marlin_server_enum(marlin_server_types.phases_selftest)
    check_marlin_server_enum(marlin_server_types.phases_crash_recovery)
    check_marlin_server_enum(marlin_server_types.phases_quick_pause)
    check_marlin_server_enum(marlin_server_types.phases_warning)
    check_marlin_server_enum(marlin_server_types.phase_cold_pull)
    check_marlin_server_enum(marlin_server_types.phases_phase_stepping)
    check_marlin_server_enum(marlin_server_types.phases_printing)
    check_marlin_server_enum(marlin_server_types.phase_serial_printing)


def test_fsm_responses():
    # Check fsm responses enum
    check_marlin_server_enum(marlin_server_types.fsm_response)


def test_fsm_phase_responses():
    # test that reponses for each FSM phase exported properly
    print(marlin_server_types.phases_load_unload_responses)
    print(marlin_server_types.phases_load_unload_responses)
    print(marlin_server_types.phases_preheat_responses)
    print(marlin_server_types.phases_print_preview_responses)
    print(marlin_server_types.phases_crash_recovery_responses)
    print(marlin_server_types.phases_quick_pause_responses)
    print(marlin_server_types.phases_warning_responses)
    print(marlin_server_types.phases_phase_stepping_responses)


if __name__ == "__main__":
    test_marlin_server_state()
    test_fsm_types()
    test_fsm_phase_types()
    test_fsm_responses()
    test_fsm_phase_responses()
