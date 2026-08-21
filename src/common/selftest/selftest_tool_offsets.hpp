#pragma once

#include "i_selftest_part.hpp"
#include "selftest_tool_offsets_config.hpp"

#include <all_tools_quick_cooling_manager.hpp>

namespace selftest {

class CSelftestPart_ToolOffsets {
public:
    CSelftestPart_ToolOffsets(IPartHandler &state_machine, const ToolOffsetsConfig_t &config, SelftestToolOffsets_t &result);

    LoopResult state_ask_user_confirm_start();
    LoopResult state_clean_nozzle_start();
    LoopResult state_move_away();
    LoopResult state_clean_nozzle();
    LoopResult state_ask_user_install_sheet();
    LoopResult state_wait_user();
    LoopResult state_home_park();
    LoopResult state_wait_moves_done();
    LoopResult state_ask_user_install_pin();
    LoopResult state_wait_stable_temp();
    LoopResult state_calibrate();
    LoopResult state_finish_calibration();
    LoopResult state_final_park();
    LoopResult state_ask_user_remove_pin();

private:
    IPartHandler &state_machine;
    SelftestToolOffsets_t &result;
    const ToolOffsetsConfig_t &config;
    AllToolsQuickCoolingManager quick_cooling { SelftestToolOffsets_t::TOOL_CALIBRATION_TEMPERATURE };
};

}; // namespace selftest
