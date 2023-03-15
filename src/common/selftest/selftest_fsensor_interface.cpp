/**
 * @file selftest_axis_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_fsensor_interface.hpp"
#include "selftest_fsensor.h"
#include "selftest_sub_state.hpp"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "eeprom.h"
#include "selftest_tool_helper.hpp"
#if BOARD_IS_XLBUDDY
    #include "src/module/prusa/toolchanger.h"
#endif

namespace selftest {
static SelftestFSensor_t staticResult; // automatically initialized by PartHandler

bool phaseFSensor(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &m_pFSensor, const std::array<const FSensorConfig_t, HOTENDS> &configs) {
    for (uint i = 0; i < HOTENDS; ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (!m_pFSensor[i]) {
            // clang-format off
            m_pFSensor[i] = selftest::Factory::CreateDynamical<CSelftestPart_FSensor>(
                configs[i],
                staticResult,
                &CSelftestPart_FSensor::stateAskHaveFilamentInit,
                &CSelftestPart_FSensor::stateWaitToolPick,
                &CSelftestPart_FSensor::stateAskHaveFilament,
                &CSelftestPart_FSensor::stateAskUnloadInit,
                &CSelftestPart_FSensor::stateAskUnload,
                &CSelftestPart_FSensor::stateFilamentUnloadEnqueueGcode,
                &CSelftestPart_FSensor::stateFilamentUnloadWaitFinished,
                &CSelftestPart_FSensor::stateFilamentUnloadWaitUser,
                &CSelftestPart_FSensor::stateCalibrate,
                &CSelftestPart_FSensor::stateCalibrateWaitFinished,
                &CSelftestPart_FSensor::stateInsertionCheck,
                &CSelftestPart_FSensor::stateInsertionOkInit,
                &CSelftestPart_FSensor::stateInsertionOk,
                &CSelftestPart_FSensor::stateEnforceRemoveInit,
                &CSelftestPart_FSensor::stateEnforceRemove
            );
            // clang-format on
        }
    }

    bool in_progress = false;
    for (uint i = 0; i < HOTENDS; ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        in_progress = in_progress || m_pFSensor[i]->Loop();
    }
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), staticResult.Serialize());

    if (in_progress) {
        return true;
    }

    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    for (uint i = 0; i < HOTENDS; ++i) {
        if (!is_tool_selftest_enabled(i, tool_mask)) {
            continue;
        }

        if (i < EEPROM_MAX_TOOL_COUNT) {
            eeres.tools[i].fsensor = m_pFSensor[i]->GetResult();
        }

        delete m_pFSensor[i];
        m_pFSensor[i] = nullptr;
    }
    eeprom_set_selftest_results(&eeres);

    return false;
}
} // namespace selftest
