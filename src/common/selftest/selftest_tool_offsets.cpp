#include "selftest_tool_offsets.hpp"
#include "Marlin/src/gcode/queue.h"
#include "Marlin/src/module/stepper.h"
#include "marlin_server.hpp"
#include "selftest_tool_helper.hpp"
#include "Marlin/src/module/temperature.h"
#include <option/has_tool_offset_pin_calibration.h>
#if HAS_TOOL_OFFSET_PIN_CALIBRATION()
    #include <marlin_stubs/G425.hpp>
#endif
#include <tool_index.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

namespace {
/// @brief Set temperature to all enabled tools
void set_nozzle_temps(int16_t temp) {
    for (auto tool : PhysicalToolIndex::all()) {
        if (is_tool_selftest_enabled(tool, AllTools {})) { // set temperature on all tools, its not possible to calibrate just one tool
            thermalManager.setTargetHotend(temp, tool);
        }
    }
}

/// @brief Check temperature of all enabled tools is at target
bool all_nozzles_at_target() {
    for (auto tool : PhysicalToolIndex::all()) {
        if (is_tool_selftest_enabled(tool, AllTools {})) { // check temperature on all tools, its not possible to calibrate just one tool
            if (!Hotend::for_tool(tool).is_nozzle_temp_reached()) {
                return false;
            }
        }
    }
    return true;
}
}; // namespace

CSelftestPart_ToolOffsets::CSelftestPart_ToolOffsets(IPartHandler &state_machine, const ToolOffsetsConfig_t &config, SelftestToolOffsets_t &result)
    : state_machine(state_machine)
    , result(result)
    , config(config) {}

LoopResult CSelftestPart_ToolOffsets::state_ask_user_confirm_start() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_confirm_start);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_clean_nozzle_start() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_move_away);
    disable_all_steppers(); // Let the user operate tools, pull out the filament if required
    set_nozzle_temps(0);

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_move_away() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold);
    // we'll ask user to clean nozzle and put on sheet - so give him some space
    do_z_clearance(100);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_clean_nozzle() {
    const auto button_pressed = state_machine.GetButtonPressed();

    if (button_pressed == Response::Continue) {
        set_nozzle_temps(SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE);
        quick_cooling.set_active(true);
        return LoopResult::RunNext;
    }

    if (IPartHandler::GetFsmPhase() == PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot) {
        // nozzle is hot or heating up
        if (button_pressed == Response::Cooldown) {
            IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold);
            set_nozzle_temps(SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE);
            quick_cooling.set_active(true);
        }
    } else if (IPartHandler::GetFsmPhase() == PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold) {
        // nozzle is cold or cooling down
        if (button_pressed == Response::Heatup) {
            IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot);
            set_nozzle_temps(SelftestToolOffsets_t::TOOL_OFFSET_CLEANING_TEMPERATURE);
            quick_cooling.set_active(false);
        }
    }

    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_ToolOffsets::state_ask_user_install_sheet() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_install_sheet);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_wait_user() {
    if (state_machine.GetButtonPressed() != Response::Continue) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_home_park() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_pin_install_prepare);

    // Ensure tool will not hit calibration pin once installed
    marlin_server::enqueue_gcode("G1 G91");
    marlin_server::enqueue_gcode("G1 Z30");
    marlin_server::enqueue_gcode("G1 G90");

    // Ensure tool 0 is picked (no risky toolchange is needed with calibration pin installed)
    marlin_server::enqueue_gcode("T0 S1 L0 D0");
    marlin_server::enqueue_gcode("G28 O");

    // Park the nozzle for easier sheet removal
    marlin_server::enqueue_gcode_printf("T%d L0 D0", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_wait_moves_done() {
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_ask_user_install_pin() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_install_pin);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_wait_stable_temp() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_stable_temp);
    if (all_nozzles_at_target()) {
        quick_cooling.set_active(false);
        return LoopResult::RunNext;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_ToolOffsets::state_calibrate() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_calibrate);
    return LoopResult::RunNext;
}

/**
 * This state exists just because full_calibration() is a blocking call and we need to update FSM state
 * to let the user know that calibration is in progress.
 * The issue is that the fsm takes update after returning from a state function, so we cannot do it in one state.
 */
LoopResult CSelftestPart_ToolOffsets::state_finish_calibration() {
    bool calibration_success = full_calibration();
    if (!calibration_success) {
        return LoopResult::Fail;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_final_park() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_move_away);
    // Let user uninstall the pin
    marlin_server::enqueue_gcode("P0 S1"); // Park tool
    marlin_server::enqueue_gcode("G27"); // Park head
    marlin_server::enqueue_gcode("M18"); // Disable steppers
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_ToolOffsets::state_ask_user_remove_pin() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_remove_pin);
    return LoopResult::RunNext;
}
