#include "selftest_tool_offsets.hpp"
#include "Marlin/src/gcode/queue.h"
#include "Marlin/src/module/stepper.h"
#include "marlin_server.hpp"
#include "selftest_tool_helper.hpp"
#include "Marlin/src/module/temperature.h"
#include "fanctl.hpp"

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

namespace {
/// @brief Set temperature to all enabled tools
void set_nozzle_temps(int16_t temp) {
    for (uint8_t tool_nr = 0; tool_nr < HOTENDS; tool_nr++) {
        if (is_tool_selftest_enabled(tool_nr, 0xFF)) { // set temperature on all tools, its not possible to calibrate just one tool
            thermalManager.setTargetHotend(temp, tool_nr);
            marlin_server::set_temp_to_display(temp, tool_nr);
        }
    }
}

/// @brief Check temperature of all enabled tools is at target
bool all_nozzles_at_target() {
    for (uint8_t tool_nr = 0; tool_nr < HOTENDS; tool_nr++) {
        if (is_tool_selftest_enabled(tool_nr, 0xFF)) { // check temperature on all tools, its not possible to calibrate just one tool
            if (thermalManager.still_heating(tool_nr)) {
                return false;
            }
        }
    }
    return true;
}
}; // namespace

/// @brief Helper class that turns fans to 100% on when cooldown is needed, and allows to reset fans back to normal control
class FanCoolingManager {
public:
    /// Request cooldown on all tools
    static void cooldown() {
        for (uint8_t tool_nr = 0; tool_nr < HOTENDS; tool_nr++) {
            if (is_tool_selftest_enabled(tool_nr, 0xFF) && thermalManager.degHotend(tool_nr) > SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE && // tool is hot
                !tool_cooling_down[tool_nr]) { // cooling is not already turned on

                start_cooling(tool_nr);
            }
        }
    }

    /// manage cooling down (to be called periodically)

    static void manage() {
        // periodically check if tool is cooled down, stop fans
        for (uint8_t tool_nr = 0; tool_nr < HOTENDS; tool_nr++) {
            if (is_tool_selftest_enabled(tool_nr, 0xFF) && // manage temperature on all tools, its not possible to calibrate just one tool
                thermalManager.degHotend(tool_nr) <= SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE && tool_cooling_down[tool_nr]) {
                stop_cooling(tool_nr);
            }
        }
    }

    /// When cooldown is active, reset it and go back to normal fan operation
    static void reset() {
        for (uint8_t tool_nr = 0; tool_nr < HOTENDS; tool_nr++) {
            if (is_tool_selftest_enabled(tool_nr, 0xFF) && // manage temperature on all tools, its not possible to calibrate just one tool
                tool_cooling_down[tool_nr]) { // tool is cooling down

                stop_cooling(tool_nr);
            }
        }
    }

private:
    static bool tool_cooling_down[HOTENDS];

    static void start_cooling(uint8_t tool_nr) {
        tool_cooling_down[tool_nr] = true;
        Fans::print(tool_nr).EnterSelftestMode();
        Fans::heat_break(tool_nr).EnterSelftestMode();
        Fans::print(tool_nr).SelftestSetPWM(255);
        Fans::heat_break(tool_nr).SelftestSetPWM(255);
    }

    static void stop_cooling(uint8_t tool_nr) {
        tool_cooling_down[tool_nr] = false;
        Fans::print(tool_nr).ExitSelftestMode();
        Fans::heat_break(tool_nr).ExitSelftestMode();
    }
};

bool FanCoolingManager::tool_cooling_down[HOTENDS] = { false };

CSelftestPart_ToolOffsets::CSelftestPart_ToolOffsets(IPartHandler &state_machine, const ToolOffsetsConfig_t &config, SelftestToolOffsets_t &result)
    : state_machine(state_machine)
    , result(result)
    , config(config) {}

CSelftestPart_ToolOffsets::~CSelftestPart_ToolOffsets() {
    FanCoolingManager::reset();
}

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
        FanCoolingManager::cooldown();
        return LoopResult::RunNext;
    }

    if (IPartHandler::GetFsmPhase() == PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot) {
        // nozzle is hot or heating up
        if (button_pressed == Response::Cooldown) {
            IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold);
            set_nozzle_temps(SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE);
            FanCoolingManager::cooldown();
        }
    } else if (IPartHandler::GetFsmPhase() == PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_cold) {
        // nozzle is cold or cooling down
        if (button_pressed == Response::Heatup) {
            IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_user_clean_nozzle_hot);
            set_nozzle_temps(SelftestToolOffsets_t::TOOL_OFFSET_CLEANING_TEMPERATURE);
            FanCoolingManager::reset();
        }
    }

    FanCoolingManager::manage();

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

    // Ensure steppers keep enabled after homing, avoid re-home after sheet removal.
    marlin_server::enqueue_gcode("M18 S0");

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
        FanCoolingManager::reset();
        return LoopResult::RunNext;
    }
    FanCoolingManager::manage();
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_ToolOffsets::state_calibrate() {
    IPartHandler::SetFsmPhase(PhasesSelftest::ToolOffsets_wait_calibrate);
    marlin_server::enqueue_gcode("G425");
    marlin_server::enqueue_gcode_printf("M18 S%d", DEFAULT_STEPPER_DEACTIVE_TIME);
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
