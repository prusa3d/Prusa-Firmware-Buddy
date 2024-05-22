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
#include "selftest_tool_helper.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif
#include <config_store/store_instance.hpp>

namespace selftest {

static std::array<SelftestLoadcell_t, HOTENDS> staticLoadCellResult;

TestReturn phaseLoadcell(const ToolMask tool_mask, std::array<IPartHandler *, HOTENDS> &m_pLoadcell, const std::span<const LoadcellConfig_t> config) {

    for (uint i = 0; i < config.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }
        if (!m_pLoadcell[i]) {
            // clang-format off
            m_pLoadcell[i] = selftest::Factory::CreateDynamical<CSelftestPart_Loadcell>(config[i],
                staticLoadCellResult[i],
                &CSelftestPart_Loadcell::stateMoveUp, &CSelftestPart_Loadcell::stateMoveUpInit, &CSelftestPart_Loadcell::stateMoveUpWaitFinish,
                &CSelftestPart_Loadcell::stateCooldownInit, &CSelftestPart_Loadcell::stateCooldown, &CSelftestPart_Loadcell::stateCooldownDeinit,
                &CSelftestPart_Loadcell::stateToolSelectInit, &CSelftestPart_Loadcell::stateToolSelectWaitFinish,
                &CSelftestPart_Loadcell::stateConnectionCheck,
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
    FSM_CHANGE_WITH_DATA__LOGGING(IPartHandler::GetFsmPhase(), staticLoadCellResult[current_tool].Serialize());

    if (in_progress) {
        return true;
    }

    bool skipped = false; ///< Return value whether to run next test
    SelftestResult eeres = config_store().selftest_result.get();
    for (uint i = 0; i < m_pLoadcell.size(); ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        // Store loadcell test state
        // Do not store if test was successful and now aborted, do not regress
        if (i < config_store_ns::max_tool_count
            && !(eeres.tools[i].tooloffset == TestResult_Passed && m_pLoadcell[i]->GetResult() == TestResult_Skipped)) {
            eeres.tools[i].loadcell = m_pLoadcell[i]->GetResult();
        }

        // If any test failed, do not run next test
        if (m_pLoadcell[i]->GetResult() != TestResult_Passed) {
            skipped = true;
        }

        delete m_pLoadcell[i];
        m_pLoadcell[i] = nullptr;
    }
    config_store().selftest_result.set(eeres);

    return TestReturn(false, skipped);
}
} // namespace selftest
