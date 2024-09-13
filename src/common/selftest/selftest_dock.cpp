#include "selftest_dock.h"
#include <module/planner.h>
#include <gcode/queue.h>
#include <module/stepper.h>
#include <module/endstops.h>
#include <module/tool_change.h>
#include <module/prusa/homing_corexy.hpp>
#include "bsod.h"
#include "marlin_server.hpp"

using buddy::puppies::Dwarf;
using buddy::puppies::dwarfs;
using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_Dock::CSelftestPart_Dock(IPartHandler &state_machine, const DockConfig_t &config,
    SelftestDock_t &result)
    : state_machine(state_machine)
    , config(config)
    , result(result)
    , dwarf(prusa_toolchanger.getTool(config.dock_id)) {}

CSelftestPart_Dock::~CSelftestPart_Dock() {
    HOTEND_LOOP() {
        prusa_toolchanger.getTool(e).set_cheese_led(); // Default LED config
    }
    prusa_toolchanger.init(false); // Ensure picked/active tool matches the reality
    toolcheck_reenable();
}

LoopResult CSelftestPart_Dock::state_ask_user_needs_calibration() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_needs_calibration);
    return LoopResult::RunNext;
}

void CSelftestPart_Dock::prepare_homing() {
    // Enable endstops in case we are close to Z max
    endstops.enable(true);
    endstops.validate_homing_move();

    // Sync position
    set_current_from_steppers();
    sync_plan_position();

    // Disable stealthChop if used. Enable diag1 pin on driver.
#if ENABLED(SENSORLESS_HOMING)
    start_sensorless_homing_per_axis(AxisEnum::Z_AXIS);
#endif
}

LoopResult CSelftestPart_Dock::stateMoveAwayInit() {
    planner.synchronize(); // Finish current move

    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_move_away); // Set moving down screen

    prepare_homing(); // Prepare homing to terminate next move as soon as endstop hits

    // Start move to z_extra_pos
    if (config.z_extra_pos > current_position.z) {
        current_position.z = config.z_extra_pos;
        line_to_current_position(config.z_extra_pos_fr);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::stateMoveAwayWait() {
    if (planner.processing()) {
        return LoopResult::RunCurrent; // Wait while moving
    }
    endstops.not_homing(); // Revert endstops to global state
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_initiate_manual_park() {
    // Manual toolchange follows, let's not interfere
    toolcheck_disable();

    // Handle parking of the possibly picked tool
    // (possibly different from the dock being calibrated)
    needs_manual_park = (prusa_toolchanger.detect_tool_nr() != PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    if (needs_manual_park) {
        // Disable steppers - let user move the head
        marlin_server::enqueue_gcode("M18 XY");

        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park1);
        result.progress = 10;
    }

    return LoopResult::RunNext;
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

    if (prusa_toolchanger.detect_tool_nr() != PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        log_error(Selftest, "User failed to park the current tool %d", config.dock_id);
        return LoopResult::GoToMark0;
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_wait_user_manual_park3() {
    if (needs_manual_park && state_machine.GetButtonPressed() != Response::Continue) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_park3);
        return LoopResult::RunCurrent;
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

    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_remove_pin() {
    result.progress = 30;

    // Disable automatic toolchange - we are going to do it manually
    toolcheck_disable();

    // Select the tool to mark it, unselect all others
    for (uint i = 0; i < HOTENDS; ++i) {
        prusa_toolchanger.getTool(i).set_selected(i == config.dock_id);
        prusa_toolchanger.getTool(i).set_cheese_led(0xff, 0x00); // LED on on the selected tool
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
    if (!dwarf.is_picked() || !dwarf.is_parked()) {
        log_error(Selftest, "Tool %d not picked and parked while dock being tightened", config.dock_id);
        return LoopResult::Fail;
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_tighten_top_screw);
    result.progress = 50;
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_measure() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_measure);

    // Assumes user just positioned head to dock position by hand

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
    if (!prusa_toolchanger.is_tool_info_valid(dwarf, tool_calibration)) {
        log_error(
            Selftest,
            "Dock %d position %f, %f differs too much from expected",
            config.dock_id, static_cast<double>(tool_calibration.dock_x), static_cast<double>(tool_calibration.dock_y));
        fatal_error(ErrCode::ERR_MECHANICAL_DOCK_POSITION_OUT_OF_BOUNDS, (config.dock_id + 1));
    }

    // Apply tool info
    old_tool_calibration = prusa_toolchanger.get_tool_info(dwarf);
    prusa_toolchanger.set_tool_info(dwarf, tool_calibration);

    result.progress = 70;

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_tighten_screw() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_tighten_bottom_screw);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_ask_user_install_pins() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_wait_user_install_pins);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_check_todock() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_park_test);

    ///@note Cannot use G-code in these steps as we need to control native machine coordinates.
    // Go in front of dock
    current_position.x = prusa_toolchanger.get_tool_info(dwarf, false).dock_x + PrusaToolChanger::PARK_X_OFFSET_1;
    current_position.y = PrusaToolChanger::SAFE_Y_WITH_TOOL;
    line_to_current_position(MMM_TO_MMS(XY_PROBE_SPEED_INITIAL));

    // Go to the back of the dock
    current_position.y = prusa_toolchanger.get_tool_info(dwarf, false).dock_y;
    line_to_current_position(MMM_TO_MMS(XY_PROBE_SPEED_INITIAL));
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_check_unlock() {
    // Set motor current and stall sensitivity to parking and remember old value
    auto x_current_ma = stepperX.rms_current();
    auto x_stall_sensitivity = stepperX.stall_sensitivity();
    auto y_current_ma = stepperY.rms_current();
    auto y_stall_sensitivity = stepperY.stall_sensitivity();
    stepperX.rms_current(PrusaToolChanger::PARKING_CURRENT_MA);
    stepperX.stall_sensitivity(PrusaToolChanger::PARKING_STALL_SENSITIVITY);
    stepperY.rms_current(PrusaToolChanger::PARKING_CURRENT_MA);
    stepperY.stall_sensitivity(PrusaToolChanger::PARKING_STALL_SENSITIVITY);

    // Unlock the clamps
    current_position.x = prusa_toolchanger.get_tool_info(dwarf, false).dock_x + PrusaToolChanger::PARK_X_OFFSET_2;
    line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);

    const auto original_acceleration = planner.settings.travel_acceleration;
    {
        auto s = planner.user_settings;
        s.travel_acceleration = PrusaToolChanger::SLOW_ACCELERATION_MM_S2;
        planner.apply_settings(s);
    }

    current_position.x = prusa_toolchanger.get_tool_info(dwarf, false).dock_x + PrusaToolChanger::PARK_X_OFFSET_3;
    line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);

    /// @note This synchronization shouldn't be delegated to state_wait_moves_done().
    ///  User could abort and acceleration and current wouldn't be reverted.
    planner.synchronize();

    // Back to high acceleration
    {
        auto s = planner.user_settings;
        s.travel_acceleration = original_acceleration;
        planner.apply_settings(s);
    }

    // Reset motor current and stall sensitivity to old value
    stepperX.rms_current(x_current_ma);
    stepperX.stall_sensitivity(x_stall_sensitivity);
    stepperY.rms_current(y_current_ma);
    stepperY.stall_sensitivity(y_stall_sensitivity);

    // Return a bit to the exact dock position
    current_position.x = prusa_toolchanger.get_tool_info(dwarf, false).dock_x;
    line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);

    // Prepare wait for sensors
    move_required.parked = true;
    move_required.picked = true;
    move_required.timeout_valid = false; // Timeout will be set when the move is finished
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_check_away() {
    prepare_homing(); // Prepare homing to terminate next move as soon as endstop hits

    // Move away
    current_position.y = PrusaToolChanger::SAFE_Y_WITHOUT_TOOL;
    line_to_current_position(PrusaToolChanger::SLOW_MOVE_MM_S);

    // Prepare wait for sensors
    move_required.parked = true;
    move_required.picked = false;
    move_required.timeout_valid = false; // Timeout will be set when the move is finished
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_check_state() {
    endstops.not_homing(); // Restore endstop state after previous stages

    // Prepare wait timeout
    if (!move_required.timeout_valid) {
        move_required.timeout_start = ticks_ms();
        move_required.timeout_valid = true;
    }

    // Check if sensors are in correct state
    if (dwarf.is_parked() == move_required.parked && dwarf.is_picked() == move_required.picked) {
        return LoopResult::RunNext;
    }

    // Check timeout
    if ((ticks_ms() - move_required.timeout_start) > PrusaToolChanger::WAIT_TIME_TOOL_PARKED_PICKED) {
        log_error(Selftest, "Tool %d didn't respond properly while self-testing park", config.dock_id);
        revert_tool_info();
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_failed);
        return LoopResult::Fail;
    }

    // Try again
    if (dwarf.refresh_park_pick_status()) {
        return LoopResult::RunCurrent;
    } else {
        log_error(Selftest, "Failed to read tool %d pick/park state", config.dock_id);
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_failed);
        return LoopResult::Fail;
    }
}

LoopResult CSelftestPart_Dock::state_selftest_entry() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_park_test);
    result.progress = 85 + 15 * (NUM_PARK_PICK_CYCLES - remaining_park_unpark_cycles) / NUM_PARK_PICK_CYCLES;
    remaining_park_unpark_cycles--;

    // Reenable automatic toolchange and toolchanger itself
    toolcheck_reenable();

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_pick() {

    // Check tool parked before picking it
    if (dwarf.is_picked() || !dwarf.is_parked()) {
        log_error(Selftest, "Tool %d not parked while self-testing pick", config.dock_id);
        revert_tool_info();
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_failed);
        return LoopResult::Fail;
    }

    marlin_server::enqueue_gcode_printf("T%d S1 L0 D0", config.dock_id);
    marlin_server::enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_park() {

    // Check tool picked before parking it
    if (!dwarf.is_picked() || dwarf.is_parked()) {
        log_error(Selftest, "Tool %d not picked while self-testing park", config.dock_id);
        revert_tool_info();
        IPartHandler::SetFsmPhase(PhasesSelftest::Dock_selftest_failed);
        return LoopResult::Fail;
    }

    marlin_server::enqueue_gcode_printf("T%d S1 L0 D0", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    marlin_server::enqueue_gcode("M400");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Dock::state_selftest_leave() {
    if (remaining_park_unpark_cycles) {
        return LoopResult::GoToMark1;
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
    if (queue.has_commands_queued() || planner.processing()) {
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
    prusa_toolchanger.set_tool_info(dwarf, old_tool_calibration);
}
