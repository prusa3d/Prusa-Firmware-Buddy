#include "selftest_part.hpp"
#include "selftest_dock.h"
#include "selftest_dock_type.hpp"
#include "src/module/prusa/toolchanger.h"
#include "eeprom.h"
#include "selftest_tool_helper.hpp"

namespace selftest {

std::array<SelftestDock_t, HOTENDS> staticResultDocks;

bool phaseDocks(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &pDocks, const std::array<const DockConfig_t, HOTENDS> &configs) {
    for (uint i = 0; i < pDocks.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (!pDocks[i]) {
            pDocks[i] = selftest::Factory::CreateDynamical<CSelftestPart_Dock>(
                configs[i],
                staticResultDocks[i],
                // Initial skip
                &CSelftestPart_Dock::state_ask_user_needs_calibration,
                &CSelftestPart_Dock::state_wait_user,
                // Manual park picked tool
                &CSelftestPart_Dock::state_initiate_manual_park,
                &CSelftestPart_Dock::state_wait_user_manual_park1,
                &CSelftestPart_Dock::state_wait_user_manual_park2,
                &CSelftestPart_Dock::state_wait_user_manual_park3,
                // Remove pins, loosen pillar
                &CSelftestPart_Dock::state_ask_user_remove_pin,
                &CSelftestPart_Dock::state_wait_user,
                &CSelftestPart_Dock::state_ask_user_loosen_pillar,
                &CSelftestPart_Dock::state_wait_user,
                &CSelftestPart_Dock::state_ask_user_lock_tool,
                &CSelftestPart_Dock::state_wait_user,
                // Fasten pillar top
                &CSelftestPart_Dock::state_hold_position,
                &CSelftestPart_Dock::state_ask_user_tighten_pillar,
                &CSelftestPart_Dock::state_wait_user,
                // Masure dock position
                &CSelftestPart_Dock::state_measure,
                &CSelftestPart_Dock::state_wait_moves_done,
                &CSelftestPart_Dock::state_compute_position,
                // Tighten rest, install pins
                &CSelftestPart_Dock::state_ask_user_install_pins,
                &CSelftestPart_Dock::state_wait_user,
                &CSelftestPart_Dock::state_ask_user_tighten_screw,
                &CSelftestPart_Dock::state_wait_user,
                // Park/unpark loop
                &CSelftestPart_Dock::state_selftest_entry,
                &CSelftestPart_Dock::state_selftest_park,
                &CSelftestPart_Dock::state_wait_moves_done,
                &CSelftestPart_Dock::state_selftest_pick,
                &CSelftestPart_Dock::state_wait_moves_done,
                &CSelftestPart_Dock::state_selftest_leave,
                // Final park
                &CSelftestPart_Dock::state_selftest_park,
                &CSelftestPart_Dock::state_wait_moves_done,
                // Final save calibration
                &CSelftestPart_Dock::state_selftest_move_away,
                &CSelftestPart_Dock::state_selftest_congratulate,
                &CSelftestPart_Dock::state_selftest_save_calibration);
        }
    }

    uint8_t current_dock = std::numeric_limits<uint8_t>::max();
    for (uint i = 0; i < pDocks.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (pDocks[i]->Loop()) {
            current_dock = i;
            break; // Skips next docks as long as the current one is running
        }
    }
    SelftestDocks_t result_docks(current_dock, staticResultDocks);
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result_docks.Serialize());
    if (current_dock != std::numeric_limits<uint8_t>::max()) {
        return true;
    }

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    for (uint i = 0; i < pDocks.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        // Store dock calibration state
        // Do not store if test was successful and now aborted, do not regress
        if (i < EEPROM_MAX_TOOL_COUNT
            && !(eeres.tools[i].dockoffset == TestResult_Passed && pDocks[i]->GetResult() == TestResult_Skipped)) {
            eeres.tools[i].dockoffset = pDocks[i]->GetResult();
        }

        delete pDocks[i];
        pDocks[i] = nullptr;
    }
    eeprom_set_selftest_results(&eeres);

    return false;
}
} // namespace selftest
