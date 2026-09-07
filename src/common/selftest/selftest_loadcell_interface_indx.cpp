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
#include <bsod/bsod.h>

namespace selftest {

static std::array<SelftestLoadcell_t, HOTENDS> staticLoadCellResult;

TestReturn phaseLoadcell([[maybe_unused]] const ToolMask tool_mask, std::array<IPartHandler *, PhysicalToolIndex::count> &m_pLoadcell, const std::span<const LoadcellConfig_t> config) {

    const auto maybe_tool = PhysicalToolIndex::currently_selected_opt().or_else([]() -> std::optional<PhysicalToolIndex> {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            return tool;
        }
        return std::nullopt;
    });
    if (!maybe_tool.has_value()) {
        return false;
    }
    const auto _tool_mask = *maybe_tool;

    for (uint i = 0; i < PhysicalToolIndex::count; ++i) {
        if (!is_tool_selftest_enabled(i, _tool_mask)) {
            continue;
        }
        if (!m_pLoadcell[i]) {
            // clang-format off
            m_pLoadcell[i] = selftest::Factory::CreateDynamical<CSelftestPart_Loadcell>(config[0],
                staticLoadCellResult[i],
                &CSelftestPart_Loadcell::stateParkingInit, &CSelftestPart_Loadcell::statePrepareParking, &CSelftestPart_Loadcell::stateParking,
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
        if (!is_tool_selftest_enabled(i, _tool_mask)) {
            continue;
        }
        if (!m_pLoadcell[i]) {
            bsod_unreachable();
        }

        if (m_pLoadcell[i]->Loop()) {
            current_tool = i;
            break; // Skips next docks as long as the current one is running
        }
    }

    bool in_progress = current_tool != std::numeric_limits<uint8_t>::max();

    if (in_progress) {
        marlin_server::fsm_change(IPartHandler::GetFsmPhase(), staticLoadCellResult[current_tool].Serialize());
        return true;
    }

    bool skipped = false; ///< Return value whether to run next test
    SelftestResult eeres = config_store().selftest_result.get();
    for (uint i = 0; i < m_pLoadcell.size(); ++i) {
        if (!is_tool_selftest_enabled(i, _tool_mask)) {
            continue;
        }
        if (!m_pLoadcell[i]) {
            bsod_unreachable();
        }

        // Store loadcell test state
        // Do not store if aborted, do not regress
        if (i < PhysicalToolIndex::count
            && m_pLoadcell[i]->GetResult() != TestResult::skipped) {
            eeres.set_loadcell(i, m_pLoadcell[i]->GetResult());
        }

        // If any test failed, do not run next test
        if (m_pLoadcell[i]->GetResult() != TestResult::passed) {
            skipped = true;
        }

        delete m_pLoadcell[i];
        m_pLoadcell[i] = nullptr;
    }
    config_store().selftest_result.set(eeres);

    return TestReturn(false, skipped);
}
} // namespace selftest
