#include "selftest_dock.h"
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "marlin_client.hpp"
#include "Marlin/src/module/planner.h"
#include "Marlin/src/gcode/queue.h"
#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/tool_change.h"

using buddy::puppies::Dwarf;
using buddy::puppies::dwarfs;
using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_Dock::CSelftestPart_Dock(IPartHandler &state_machine, const DockConfig_t &config,
    SelftestDock_t &result)
    : state_machine(state_machine)
    , config(config)
    , result(result) {}

CSelftestPart_Dock::~CSelftestPart_Dock() {}

LoopResult CSelftestPart_Dock::state_ask_user_needs_calibration() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_needs_calibration);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_initiate_manual_park() {
    // Handle parking of the possibly picked tool
    // (possibly different from the dock being calibrated)
    prusa_toolchanger.tool_detect();
    needs_manual_park = prusa_toolchanger.has_tool();
    if (needs_manual_park) {
        // Disable steppers - let user move the head
        marlin_server::enqueue_gcode("M18 XY");

        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park1);
        result.progress = 10;
    }

    return LoopResult::MarkLoop;
}

LoopResult CSelftestPart_Dock::state_wait_user_manual_park1() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park1);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Dock::state_wait_user_manual_park2() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park2);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Dock::state_wait_user_manual_park3() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park3);
        return LoopResult::RunCurrent;
    }
    prusa_toolchanger.tool_detect();
    if (prusa_toolchanger.has_tool()) {
        SelftestInstance().log_printf("Tool picked after manual park");
        log_error(Selftest, "User failed to park the current tool", config.dock_id);
        return LoopResult::GoToMark;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_wait_user() {
    switch (state_machine.GetButtonPressed()) {
    case Response::Continue:
        return LoopResult::RunNext;
    case Response::Abort:
        return LoopResult::Abort;
    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult CSelftestPart_Dock::state_wait_moves_done() {
    if (state_machine.GetButtonPressed() == Response::Abort) {
        return LoopResult::Abort;
    }

    if (planner.movesplanned() || queue.has_commands_queued()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_remove_pin() {
    result.progress = 30;

    // Select the tool to mark it, unselect all others
    for (uint i = 0; i < HOTENDS; ++i) {
        prusa_toolchanger.getTool(i).set_selected(i == config.dock_id);
    }

    // Disable steppers - let user operate with the printer
    marlin_server::enqueue_gcode("M18 XY");

    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_remove_pins);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Dock::state_ask_user_loosen_pillar() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_loosen_pillar);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Dock::state_ask_user_lock_tool() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_lock_tool);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_hold_position() {
    marlin_server::enqueue_gcode("M17 XY");
    marlin_server::enqueue_gcode("M18 S0"); // Ensure motors are enabled as the user fastens the screws
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_tighten_pillar() {
    Dwarf &dwarf = prusa_toolchanger.getTool(config.dock_id);
    if (!dwarf.is_picked() || !dwarf.is_parked()) {
        SelftestInstance().log_printf("Tool not picked and parked while dock being tightened");
        log_error(Selftest, "Tool %d not picked and parked while dock being tightened.", config.dock_id);
        return LoopResult::Fail;
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_tighten_top_screw);
    result.progress = 50;
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_measure() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_measure);

    // Assumes user just positioned head to dock position by hand
    prusa_toolchanger.tool_detect();

    // Remember initial stepper position
    position_before_measure = xy_long_t({ { {
        .x = stepper.position_from_startup(AxisEnum::A_AXIS),
        .y = stepper.position_from_startup(AxisEnum::B_AXIS),
    } } }); // GCC bug? (should be .a = ..., .b = ...) works with GCC 12.2.1

    marlin_server::enqueue_gcode("G91"); // Relative positioning
    // Detach from dock
    marlin_server::enqueue_gcode_printf(
        "G0 F%d X%f",
        PrusaToolChanger::FORCE_MOVE_MM_S * 60,
        static_cast<double>(X_UNLOCK_DISTANCE_MM));
    // Back in front of the dock to not bump it when homing
    marlin_server::enqueue_gcode_printf(
        "G0 F%d Y%f",
        PrusaToolChanger::FORCE_MOVE_MM_S * 60,
        static_cast<double>(PrusaToolChanger::SAFE_Y_WITH_TOOL - PrusaToolChanger::SAFE_Y_WITHOUT_TOOL));
    marlin_server::enqueue_gcode("G90"); // Absolute positioning

    // Home
    marlin_server::enqueue_gcode("G28 XY R0");
    marlin_server::enqueue_gcode("M18 S0"); // Ensure motors are enabled as the user fastens the screws

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_compute_position() {
    // Calculate x,y distance between home and dock position
    xy_long_t after = { { {
        .x = stepper.position_from_startup(AxisEnum::A_AXIS),
        .y = stepper.position_from_startup(AxisEnum::B_AXIS),
    } } }; // GCC bug? (should be .a = ..., .b = ...) works with GCC 12.2.1

    xy_pos_t diff;
    corexy_ab_to_xy(position_before_measure - after, diff);

    // Obtain dock info
    const PrusaToolInfo tool_calibration = {
        .dock_x = diff.x + current_position.x, // ~25mm + N * 82mm
        .dock_y = diff.y + current_position.y, // ~451mm
    };

    // Verify dock info
    if (!prusa_toolchanger.is_tool_info_valid(prusa_toolchanger.getTool(config.dock_id), tool_calibration)) {
        SelftestInstance().log_printf("Dock position differs too much from expected");
        log_error(
            Selftest,
            "Dock %d position %f, %f differs too much from expected.",
            config.dock_id, static_cast<double>(tool_calibration.dock_x), static_cast<double>(tool_calibration.dock_y));
        return LoopResult::Fail;
    }

    // Apply tool info
    const Dwarf &dwarf = prusa_toolchanger.getTool(config.dock_id);
    old_tool_calibration = prusa_toolchanger.get_tool_info(dwarf);
    prusa_toolchanger.set_tool_info(dwarf, tool_calibration);

    result.progress = 70;

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_install_pins() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_install_pins);
    return LoopResult::RunNext;
}
LoopResult CSelftestPart_Dock::state_ask_user_tighten_screw() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_tighten_bottom_screw);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_entry() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_park_test);
    result.progress = 85 + 15 * (NUM_PARK_PICK_CYCLES - remaining_park_unpark_cycles) / NUM_PARK_PICK_CYCLES;
    remaining_park_unpark_cycles--;
    return LoopResult::MarkLoop;
}

LoopResult CSelftestPart_Dock::state_selftest_park() {

    // Check tool picked before parking it
    prusa_toolchanger.tool_detect();
    if (active_extruder != config.dock_id) {
        SelftestInstance().log_printf("Tool not picked, cannot park it");
        log_error(Selftest, "Tool %d not picked while self-testing park", config.dock_id);
        revert_tool_info();
        return LoopResult::Fail;
    }

    marlin_server::enqueue_gcode_printf("T%d S1", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    marlin_server::enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_pick() {

    // Check tool picked before parking it
    prusa_toolchanger.tool_detect();
    if (prusa_toolchanger.has_tool()) {
        SelftestInstance().log_printf("Tool not parked, cannot pick it");
        log_error(Selftest, "Tool %d not parked while self-testing pick", config.dock_id);
        revert_tool_info();
        return LoopResult::Fail;
    }

    marlin_server::enqueue_gcode_printf("T%d S1", config.dock_id);
    marlin_server::enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_leave() {
    if (remaining_park_unpark_cycles) {
        return LoopResult::GoToMark;
    }
    // Re-enable motor timeout (disable due to user waits)
    marlin_server::enqueue_gcode_printf("M18 S%d", DEFAULT_STEPPER_DEACTIVE_TIME);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_move_away() {
    marlin_server::enqueue_gcode_printf(
        "G0 Y%f",
        static_cast<double>(PrusaToolChanger::SAFE_Y_WITH_TOOL / 2));
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_congratulate() {
    if (planner.movesplanned() || queue.has_commands_queued()) {
        return LoopResult::RunCurrent;
    }

    if (state_machine.GetButtonPressed() == Response::Continue) {
        return LoopResult::RunNext;
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_calibration_success);
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_Dock::state_selftest_save_calibration() {
    // Store dock info
    prusa_toolchanger.save_tool_info();

    return LoopResult::RunNext;
}

void CSelftestPart_Dock::revert_tool_info() {
    const Dwarf &dwarf = prusa_toolchanger.getTool(config.dock_id);
    prusa_toolchanger.set_tool_info(dwarf, old_tool_calibration);
}
