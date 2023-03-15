#include "selftest_kennel.h"
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "marlin_client.hpp"
#include "Marlin/src/module/planner.h"
#include "Marlin/src/gcode/queue.h"
#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/tool_change.h"

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_Kennel::CSelftestPart_Kennel(IPartHandler &state_machine, const KennelConfig_t &config,
    SelftestKennel_t &result)
    : state_machine(state_machine)
    , config(config)
    , result(result) {}

CSelftestPart_Kennel::~CSelftestPart_Kennel() {}

LoopResult CSelftestPart_Kennel::state_ask_user_needs_calibration() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_needs_calibration);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_initiate_manual_park() {
    // Handle parking of the possibly picked tool
    // (possibly different from the kennel being calibrated)
    tool_detect();
    needs_manual_park = prusa_toolchanger.has_tool();
    if (needs_manual_park) {
        // Disable steppers - let user move the head
        marlin_server_enqueue_gcode("M18 XY");

        IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_park1);
        result.progress = 10;
    }

    return LoopResult::MarkLoop;
}

LoopResult CSelftestPart_Kennel::state_wait_user_manual_park1() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_park1);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Kennel::state_wait_user_manual_park2() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_park2);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Kennel::state_wait_user_manual_park3() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_park3);
        return LoopResult::RunCurrent;
    }
    tool_detect();
    if (prusa_toolchanger.has_tool()) {
        SelftestInstance().log_printf("Tool picked after manual park");
        log_error(Selftest, "User failed to park the current tool", config.kennel_id);
        return LoopResult::GoToMark;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_wait_user() {
    switch (state_machine.GetButtonPressed()) {
    case Response::Continue:
        return LoopResult::RunNext;
    case Response::Abort:
        return LoopResult::Abort;
    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult CSelftestPart_Kennel::state_initiate_pin_removal() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_pin_remove_prepare);
    result.progress = 30;

    if (needs_manual_park) {
        // Move a bit back to ensure the head is not locked in the tool
        marlin_server_enqueue_gcode("G91"); // Relative positioning
        marlin_server_enqueue_gcode_printf(
            "G0 F%d Y%f",
            PrusaToolChanger::TRAVEL_MOVE_MM_S * 60,
            static_cast<double>(PrusaToolChanger::SAFE_Y_WITH_TOOL - PrusaToolChanger::SAFE_Y_WITHOUT_TOOL));
        marlin_server_enqueue_gcode("G90"); // Absolute positioning
    }

    // Select the tool to mark it, unselect all others
    for (uint i = 0; i < HOTENDS; ++i) {
        prusa_toolchanger.getTool(i).set_selected(i == config.kennel_id);
    }

    // Disable steppers - let use move the head
    marlin_server_enqueue_gcode("M18 XY");

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_wait_moves_done() {
    if (state_machine.GetButtonPressed() == Response::Abort) {
        return LoopResult::Abort;
    }

    if (planner.movesplanned() || queue.has_commands_queued()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_ask_user_remove_pin() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_remove_pins);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Kennel::state_ask_user_loosen_pillar() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_loosen_pillar);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Kennel::state_ask_user_lock_tool() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_lock_tool);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_hold_position() {
    marlin_server_enqueue_gcode("M17 XY");
    marlin_server_enqueue_gcode("M18 S0"); // Ensure motors are enabled as the user fastens the screws
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_ask_user_tighten_pillar() {
    Dwarf &dwarf = prusa_toolchanger.getTool(config.kennel_id);
    if (!dwarf.is_picked() || !dwarf.is_parked()) {
        SelftestInstance().log_printf("Tool not picked and parked while kennel being tightened");
        log_error(Selftest, "Tool %d not picked and parked while kennel being tightened.", config.kennel_id);
        return LoopResult::Fail;
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_tighten_top_screw);
    result.progress = 50;
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_measure() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_measure);

    // Assumes user just positioned head to kennel position by hand
    tool_detect();

    // Remember initial stepper position
    position_before_measure = xy_long_t({ { {
        .x = stepper.position_from_startup(AxisEnum::A_AXIS),
        .y = stepper.position_from_startup(AxisEnum::B_AXIS),
    } } }); // GCC bug? (should be .a = ..., .b = ...) works with GCC 12.2.1

    marlin_server_enqueue_gcode("G91"); // Relative positioning
    // Detach from kennel
    marlin_server_enqueue_gcode_printf(
        "G0 F%d X%f",
        PrusaToolChanger::FORCE_MOVE_MM_S * 60,
        static_cast<double>(X_UNLOCK_DISTANCE_MM));
    // Back in front of the kennel to not bump it when homing
    marlin_server_enqueue_gcode_printf(
        "G0 F%d Y%f",
        PrusaToolChanger::FORCE_MOVE_MM_S * 60,
        static_cast<double>(PrusaToolChanger::SAFE_Y_WITH_TOOL - PrusaToolChanger::SAFE_Y_WITHOUT_TOOL));
    marlin_server_enqueue_gcode("G90"); // Absolute positioning

    // Home
    marlin_server_enqueue_gcode("G28 XY R0");
    marlin_server_enqueue_gcode("M18 S0"); // Ensure motors are enabled as the user fastens the screws

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_compute_position() {
    // Calculate x,y distance between home and kennel position
    xy_long_t after = { { {
        .x = stepper.position_from_startup(AxisEnum::A_AXIS),
        .y = stepper.position_from_startup(AxisEnum::B_AXIS),
    } } }; // GCC bug? (should be .a = ..., .b = ...) works with GCC 12.2.1

    xy_pos_t diff;
    corexy_ab_to_xy(position_before_measure - after, diff);

    // Obtain kennel info
    const PrusaToolInfo tool_calibration = {
        .kennel_x = diff.x + current_position.x, // ~25mm + N * 82mm
        .kennel_y = diff.y + current_position.y, // ~451mm
    };

    // Verify kennel info
    if (!prusa_toolchanger.is_tool_info_valid(prusa_toolchanger.getTool(config.kennel_id), tool_calibration)) {
        SelftestInstance().log_printf("Kennel position differs too much from expected");
        log_error(
            Selftest,
            "Kennel %d position %f, %f differs too much from expected.",
            config.kennel_id, static_cast<double>(tool_calibration.kennel_x), static_cast<double>(tool_calibration.kennel_y));
        return LoopResult::Fail;
    }

    // Apply tool info
    const Dwarf &dwarf = prusa_toolchanger.getTool(config.kennel_id);
    old_tool_calibration = prusa_toolchanger.get_tool_info(dwarf);
    prusa_toolchanger.set_tool_info(dwarf, tool_calibration);

    result.progress = 70;

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_ask_user_install_pins() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_install_pins);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Kennel::state_ask_user_tighten_screw() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_wait_user_tighten_bottom_screw);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_selftest_entry() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Kennel_selftest_park_test);
    result.progress = 85 + 15 * (NUM_PARK_PICK_CYCLES - remaining_park_unpark_cycles) / NUM_PARK_PICK_CYCLES;
    remaining_park_unpark_cycles--;
    return LoopResult::MarkLoop;
}

LoopResult CSelftestPart_Kennel::state_selftest_park() {

    // Check tool picked before parking it
    tool_detect();
    if (active_extruder != config.kennel_id) {
        SelftestInstance().log_printf("Tool not picked, cannot park it");
        log_error(Selftest, "Tool %d not picked while self-testing park", config.kennel_id);
        revert_tool_info();
        return LoopResult::Fail;
    }

    marlin_server_enqueue_gcode_printf("T%d S1", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    marlin_server_enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_selftest_pick() {

    // Check tool picked before parking it
    tool_detect();
    if (prusa_toolchanger.has_tool()) {
        SelftestInstance().log_printf("Tool not parked, cannot pick it");
        log_error(Selftest, "Tool %d not parked while self-testing pick", config.kennel_id);
        revert_tool_info();
        return LoopResult::Fail;
    }

    marlin_server_enqueue_gcode_printf("T%d S1", config.kennel_id);
    marlin_server_enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_selftest_leave() {
    if (remaining_park_unpark_cycles) {
        return LoopResult::GoToMark;
    }
    // Re-enable motor timeout (disable due to user waits)
    marlin_server_enqueue_gcode_printf("G18 S%d", DEFAULT_STEPPER_DEACTIVE_TIME);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Kennel::state_selftest_save_calibration() {
    // Store kennel info
    prusa_toolchanger.save_tool_info();

    return LoopResult::RunNext;
}

void CSelftestPart_Kennel::revert_tool_info() {
    const Dwarf &dwarf = prusa_toolchanger.getTool(config.kennel_id);
    prusa_toolchanger.set_tool_info(dwarf, old_tool_calibration);
}
