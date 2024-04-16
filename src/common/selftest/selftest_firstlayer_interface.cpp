/**
 * @file selftest_firstlayer_interface.cpp
 */
#include "selftest_firstlayer_interface.hpp"
#include "selftest_firstlayer.hpp"
#include "selftest_sub_state.hpp"
#include "selftest_part.hpp"
#include "i_selftest.hpp"
#include <config_store/store_instance.hpp>

namespace selftest {
constexpr uint8_t UNKNOWN_SHEET = std::numeric_limits<uint8_t>::max();
SelftestFirstLayer_t staticResult; // automatically initialized by PartHandler
static FirstLayerConfig_t config { .partname = "First Layer", .previous_sheet = UNKNOWN_SHEET };
bool phaseFirstLayer(IPartHandler *&pFirstLayer, const uint8_t previous_sheet) {

    if (pFirstLayer == nullptr) {
        config.previous_sheet = previous_sheet;
        // clang-format off
        pFirstLayer = selftest::Factory::CreateDynamical<CSelftestPart_FirstLayer>(config,
            staticResult,
            &CSelftestPart_FirstLayer::stateStart, &CSelftestPart_FirstLayer::stateCycleMark,
            &CSelftestPart_FirstLayer::stateAskFilamentInit, &CSelftestPart_FirstLayer::stateAskFilament,
            &CSelftestPart_FirstLayer::statePreheatEnqueueGcode, &CSelftestPart_FirstLayer::statePreheatWaitFinished,
            &CSelftestPart_FirstLayer::stateFilamentLoadEnqueueGcode, &CSelftestPart_FirstLayer::stateFilamentLoadWaitFinished,
            &CSelftestPart_FirstLayer::stateFilamentUnloadEnqueueGcode, &CSelftestPart_FirstLayer::stateFilamentUnloadWaitFinished,
            &CSelftestPart_FirstLayer::stateShowCalibrateMsg, &CSelftestPart_FirstLayer::stateHandleNext,
            &CSelftestPart_FirstLayer::stateCycleMark,
            &CSelftestPart_FirstLayer::stateInitialDistanceInit, &CSelftestPart_FirstLayer::stateInitialDistance,
            &CSelftestPart_FirstLayer::stateShowStartPrint, &CSelftestPart_FirstLayer::stateHandleNext,
            &CSelftestPart_FirstLayer::statePrintInit, &CSelftestPart_FirstLayer::stateWaitNozzle, &CSelftestPart_FirstLayer::stateWaitBed, &CSelftestPart_FirstLayer::stateHome, &CSelftestPart_FirstLayer::stateMbl,
            &CSelftestPart_FirstLayer::statePrint, &CSelftestPart_FirstLayer::stateMblFinished, &CSelftestPart_FirstLayer::statePrintFinished,
            &CSelftestPart_FirstLayer::stateReprintInit, &CSelftestPart_FirstLayer::stateReprint,
            &CSelftestPart_FirstLayer::stateCleanSheetInit, &CSelftestPart_FirstLayer::stateCleanSheet,
            &CSelftestPart_FirstLayer::stateFinish);
        // clang-format on
    }

    bool in_progress = pFirstLayer->Loop();
    FSM_CHANGE_WITH_DATA__LOGGING(IPartHandler::GetFsmPhase(), staticResult.Serialize());

    if (in_progress) {
        return true;
    }

    config_store().run_first_layer.set(false);

    delete pFirstLayer;
    pFirstLayer = nullptr;
    config.previous_sheet = UNKNOWN_SHEET;
    return false;
}
} // namespace selftest
