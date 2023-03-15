#include "selftest_part.hpp"
#include "selftest_kennel.h"
#include "selftest_kennel_type.hpp"
#include "src/module/prusa/toolchanger.h"
#include "eeprom.h"
#include "selftest_tool_helper.hpp"

namespace selftest {

std::array<SelftestKennel_t, HOTENDS> staticResultKennels;

bool phaseKennels(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &pKennels, const std::array<const KennelConfig_t, HOTENDS> &configs) {
    for (uint i = 0; i < pKennels.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (!pKennels[i]) {
            pKennels[i] = selftest::Factory::CreateDynamical<CSelftestPart_Kennel>(
                configs[i],
                staticResultKennels[i],
                // Initial skip
                &CSelftestPart_Kennel::state_ask_user_needs_calibration,
                &CSelftestPart_Kennel::state_wait_user,
                // Manual park picked tool
                &CSelftestPart_Kennel::state_initiate_manual_park,
                &CSelftestPart_Kennel::state_wait_user_manual_park1,
                &CSelftestPart_Kennel::state_wait_user_manual_park2,
                &CSelftestPart_Kennel::state_wait_user_manual_park3,
                // Remove pins, loosen pillar
                &CSelftestPart_Kennel::state_ask_user_remove_pin,
                &CSelftestPart_Kennel::state_wait_user,
                &CSelftestPart_Kennel::state_ask_user_loosen_pillar,
                &CSelftestPart_Kennel::state_wait_user,
                &CSelftestPart_Kennel::state_ask_user_lock_tool,
                &CSelftestPart_Kennel::state_wait_user,
                // Fasten pillar top
                &CSelftestPart_Kennel::state_hold_position,
                &CSelftestPart_Kennel::state_ask_user_tighten_pillar,
                &CSelftestPart_Kennel::state_wait_user,
                // Masure kennel position
                &CSelftestPart_Kennel::state_measure,
                &CSelftestPart_Kennel::state_wait_moves_done,
                &CSelftestPart_Kennel::state_compute_position,
                // Tighten rest, install pins
                &CSelftestPart_Kennel::state_ask_user_install_pins,
                &CSelftestPart_Kennel::state_wait_user,
                &CSelftestPart_Kennel::state_ask_user_tighten_screw,
                &CSelftestPart_Kennel::state_wait_user,
                // Park/unpark loop
                &CSelftestPart_Kennel::state_selftest_entry,
                &CSelftestPart_Kennel::state_selftest_park,
                &CSelftestPart_Kennel::state_wait_moves_done,
                &CSelftestPart_Kennel::state_selftest_pick,
                &CSelftestPart_Kennel::state_wait_moves_done,
                &CSelftestPart_Kennel::state_selftest_leave,
                // Final park
                &CSelftestPart_Kennel::state_selftest_park,
                &CSelftestPart_Kennel::state_wait_moves_done,
                // Final save calibration
                &CSelftestPart_Kennel::state_selftest_save_calibration);
        }
    }

    uint8_t current_kennel = std::numeric_limits<uint8_t>::max();
    for (uint i = 0; i < pKennels.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (pKennels[i]->Loop()) {
            current_kennel = i;
            break; // Skips next kennels as long as the current one is running
        }
    }
    SelftestKennels_t result_kennels(current_kennel, staticResultKennels);
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result_kennels.Serialize());
    if (current_kennel != std::numeric_limits<uint8_t>::max()) {
        return true;
    }

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    for (uint i = 0; i < pKennels.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        // Store kennel calibration state
        // Do not store if test was successful and now aborted, do not regress
        if (i < EEPROM_MAX_TOOL_COUNT
            && !(eeres.tools[i].kenneloffset == TestResult_Passed && pKennels[i]->GetResult() == TestResult_Skipped)) {
            eeres.tools[i].kenneloffset = pKennels[i]->GetResult();
        }

        delete pKennels[i];
        pKennels[i] = nullptr;
    }
    eeprom_set_selftest_results(&eeres);

    return false;
}
} // namespace selftest
