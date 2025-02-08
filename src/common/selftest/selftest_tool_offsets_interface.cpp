#include "selftest_tool_offsets_interface.hpp"
#include "selftest_part.hpp"
#include "selftest_tool_offsets.hpp"
#include <module/prusa/toolchanger.h>
#include <config_store/store_instance.hpp>

namespace selftest {

namespace {
    SelftestToolOffsets_t staticResultToolOffsets;
}

TestReturn phaseToolOffsets([[maybe_unused]] const ToolMask tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config) {
    std::bitset<config_store_ns::max_tool_count> selected_tools = (1 << config_store_ns::max_tool_count) - 1;
    return phaseToolOffset(tool_mask, pToolOffsets, config, selected_tools);
}

TestReturn phaseToolOffset([[maybe_unused]] const ToolMask tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config, std::bitset<config_store_ns::max_tool_count> selected_tools) {
    if (!pToolOffsets) {
        pToolOffsets = selftest::Factory::CreateDynamical<CSelftestPart_ToolOffsets>(
            config,
            staticResultToolOffsets,
            &CSelftestPart_ToolOffsets::state_ask_user_confirm_start,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_clean_nozzle_start,
            &CSelftestPart_ToolOffsets::state_move_away,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_clean_nozzle,
            &CSelftestPart_ToolOffsets::state_ask_user_install_sheet,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_home_park,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_ask_user_install_pin,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_wait_stable_temp,
            &CSelftestPart_ToolOffsets::state_calibrate,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_final_park,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_ask_user_remove_pin,
            &CSelftestPart_ToolOffsets::state_wait_user);
    }

    const bool in_progress = pToolOffsets->Loop();
    SelftestToolOffsets_t result_tool_offsets(staticResultToolOffsets);
    marlin_server::fsm_change(IPartHandler::GetFsmPhase(), result_tool_offsets.Serialize());
    if (in_progress) {
        return true;
    }

    SelftestResult eeres = config_store().selftest_result.get();
    for (size_t i = 0; i < selected_tools.size(); ++i) {
        size_t current_tool = selected_tools[i];
        if (current_tool) {
            if (!prusa_toolchanger.is_tool_enabled(i)) {
                continue; // Tool is not enabled
            }

            if (eeres.tools[i].tooloffset == TestResult_Passed
                && pToolOffsets->GetResult() == TestResult_Skipped) {
                continue; // Test was successful and now aborted, do not regress
            }

            // Store tool calibration state
            eeres.tools[i].tooloffset = pToolOffsets->GetResult();
        }
    }
    config_store().selftest_result.set(eeres);

    const bool skipped = pToolOffsets->GetResult() != TestResult_Passed; ///< Return value whether to run next test

    delete pToolOffsets;
    pToolOffsets = nullptr;
    return TestReturn(false, skipped);
}

} // namespace selftest
