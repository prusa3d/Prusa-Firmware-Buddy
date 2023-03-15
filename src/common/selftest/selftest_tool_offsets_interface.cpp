#include "selftest_part.hpp"
#include "selftest_tool_offsets.hpp"
#include "eeprom.h"

namespace selftest {

namespace {
    SelftestToolOffsets_t staticResultToolOffsets;
}

bool phaseToolOffsets(const uint8_t tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config) {
    if (!pToolOffsets) {
        pToolOffsets = selftest::Factory::CreateDynamical<CSelftestPart_ToolOffsets>(
            config,
            staticResultToolOffsets,
            &CSelftestPart_ToolOffsets::state_ask_user_confirm_start,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_clean_nozzle_start,
            &CSelftestPart_ToolOffsets::state_clean_nozzle,
            &CSelftestPart_ToolOffsets::state_ask_user_install_sheet,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_home_park,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_ask_user_install_pin,
            &CSelftestPart_ToolOffsets::state_wait_user,
            &CSelftestPart_ToolOffsets::state_calibrate,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_final_park,
            &CSelftestPart_ToolOffsets::state_wait_moves_done,
            &CSelftestPart_ToolOffsets::state_ask_user_remove_pin,
            &CSelftestPart_ToolOffsets::state_wait_user);
    }

    const bool in_progress = pToolOffsets->Loop();
    SelftestToolOffsets_t result_tool_offsets(staticResultToolOffsets);
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result_tool_offsets.Serialize());
    if (in_progress) {
        return true;
    }

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    /* TODO:
    for (uint i = 0; i < pToolOffsets.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }
    */
    for (int i = 0; i < EEPROM_MAX_TOOL_COUNT; i++) {

        // Store tool calibration state
        if (i < EEPROM_MAX_TOOL_COUNT) {
            eeres.tools[i].tooloffset = pToolOffsets->GetResult();
        }

        /* TODO:
        delete pToolOffsets[i];
        pToolOffsets[i] = nullptr;
        */
    }
    eeprom_set_selftest_results(&eeres);

    delete pToolOffsets;
    pToolOffsets = nullptr;
    return false;
}

} // namespace selftest
