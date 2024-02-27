/**
 * @file selftest_hotend_specify.cpp
 */

#include "selftest_hotend_specify.hpp"
#include <guiconfig/wizard_config.hpp>
#include <config_store/store_instance.hpp>

#include <array>

using namespace selftest;

CSelftestPart_HotendSpecify::CSelftestPart_HotendSpecify(IPartHandler &state_machine, const HotendSpecifyConfig &config,
    SelftestHotendSpecifyType &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result) {
    rStateMachine.SetTimeToShowResult(0);
}

LoopResult CSelftestPart_HotendSpecify::stateStart() {
    rResult.hotend_type = config_store().hotend_type.get();
    rResult.nozzle_type = config_store().nozzle_type.get();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotendSpecify::stateAskAdjust() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Adjust:
        return LoopResult::RunNext;
    case Response::Skip:
        return LoopResult::Abort;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_HotendSpecify::stateAskHotendInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotend_type);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotendSpecify::stateAskHotend() {
    // Revisit this when new hotends are added
    static_assert(hotend_type_count == 3);

    switch (rStateMachine.GetButtonPressed()) {

    case Response::HotendType_Stock:
    case Response::No: // If only stock & sock is available, only yes/no do you have sock dialog is shown
        rResult.hotend_type = HotendType::stock;
        return LoopResult::RunNext;

    case Response::HotendType_StockWithSock:
    case Response::Yes: // If only stock & sock is available, only yes/no do you have sock dialog is shown
        rResult.hotend_type = HotendType::stock_with_sock;
        return LoopResult::RunNext;

    case Response::HotendType_E3DRevo:
        rResult.hotend_type = HotendType::e3d_revo;
        return LoopResult::RunNext;

    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult CSelftestPart_HotendSpecify::stateAskNozzleInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotend_nozzle_type);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotendSpecify::stateAskNozzle() {
    const Response response = rStateMachine.GetButtonPressed();

    // When some new nozzle type gets added, we might want to revisit this
    static_assert(size_t(NozzleType::_cnt) == 2);
    switch (response) {

    case Response::NozzleType_Normal:
        rResult.nozzle_type = NozzleType::Normal;
        return LoopResult::RunNext;

    case Response::NozzleType_HighFlow:
        rResult.nozzle_type = NozzleType::HighFlow;
        return LoopResult::RunNext;

    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_HotendSpecify::stateAskRetryInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotend_retry);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotendSpecify::stateAskRetry() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Yes:
        return LoopResult::RunNext;
    case Response::No:
        return LoopResult::Abort;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}
