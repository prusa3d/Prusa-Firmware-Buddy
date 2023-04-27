/**
 * @file selftest_loadcell_interface.cpp
 * @author Radek Vana
 * @date 2021-09-29
 */
#include "selftest_loadcell_interface.hpp"
#include "selftest_loadcell.h"
#include "selftest_loadcell_type.hpp"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "eeprom.h"
#include "selftest_tool_helper.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

namespace selftest {

static std::array<SelftestLoadcell_t, HOTENDS> staticLoadCellResult;

bool phaseLoadcell(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &m_pLoadcell, const std::span<const LoadcellConfig_t> config) {

    for (uint i = 0; i < config.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }
        if (!m_pLoadcell[i]) {
            // clang-format off
            m_pLoadcell[i] = selftest::Factory::CreateDynamical<CSelftestPart_Loadcell>(config[i],
                staticLoadCellResult[i],
                &CSelftestPart_Loadcell::stateMoveUp, &CSelftestPart_Loadcell::stateMoveUpInit, &CSelftestPart_Loadcell::stateMoveUpWaitFinish,
                &CSelftestPart_Loadcell::stateToolSelectInit, &CSelftestPart_Loadcell::stateToolSelectWaitFinish,
                &CSelftestPart_Loadcell::stateConnectionCheck,
                &CSelftestPart_Loadcell::stateCooldownInit, &CSelftestPart_Loadcell::stateCooldown, &CSelftestPart_Loadcell::stateCooldownDeinit,
                &CSelftestPart_Loadcell::stateCycleMark,
                &CSelftestPart_Loadcell::stateAskAbortInit, &CSelftestPart_Loadcell::stateAskAbort,
                &CSelftestPart_Loadcell::stateTapCheckCountDownInit, &CSelftestPart_Loadcell::stateTapCheckCountDown,
                &CSelftestPart_Loadcell::stateTapCheckInit, &CSelftestPart_Loadcell::stateTapCheck, &CSelftestPart_Loadcell::stateTapOk);
            // clang-format on
        }
    }

    uint8_t current_tool = std::numeric_limits<uint8_t>::max();
    for (uint i = 0; i < m_pLoadcell.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (m_pLoadcell[i]->Loop()) {
            current_tool = i;
            break; // Skips next docks as long as the current one is running
        }
    }

    bool in_progress = current_tool != std::numeric_limits<uint8_t>::max();
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), staticLoadCellResult[current_tool].Serialize());

    if (in_progress) {
        return true;
    }

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    for (uint i = 0; i < m_pLoadcell.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        // Store loadcell test state
        // Do not store if test was successful and now aborted, do not regress
        if (i < EEPROM_MAX_TOOL_COUNT
            && !(eeres.tools[i].tooloffset == TestResult_Passed && m_pLoadcell[i]->GetResult() == TestResult_Skipped)) {
            eeres.tools[i].loadcell = m_pLoadcell[i]->GetResult();
        }

        delete m_pLoadcell[i];
        m_pLoadcell[i] = nullptr;
    }
    eeprom_set_selftest_results(&eeres);
    return false;
}
} // namespace selftest
