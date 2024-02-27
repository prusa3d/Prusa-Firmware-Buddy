/**
 * @file selftest_hot_end_sock.cpp
 */

#include "selftest_hot_end_sock.hpp"
#include "wizard_config.hpp"
#include <config_store/store_instance.hpp>

#include <array>

using namespace selftest;

CSelftestPart_HotEndSock::CSelftestPart_HotEndSock(IPartHandler &state_machine, const HotEndSockConfig &config,
    SelftestHotEndSockType &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result) {
    rStateMachine.SetTimeToShowResult(0);
}

LoopResult CSelftestPart_HotEndSock::stateStart() {
    rResult.hotend_type = config_store().hotend_type.get();
    rResult.nozzle_type = config_store().nozzle_type.get();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotEndSock::stateAskAdjust() {
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

LoopResult CSelftestPart_HotEndSock::stateAskSockInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotEnd_sock);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotEndSock::stateAskSock() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Yes:
        rResult.hotend_type = HotendType::stock_with_sock;
        return LoopResult::RunNext;
    case Response::No:
        rResult.hotend_type = HotendType::stock;
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_HotEndSock::stateAskNozzleInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotEnd_nozzle_type);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotEndSock::stateAskNozzle() {
    const Response response = rStateMachine.GetButtonPressed();

    // When some new nozzle type gets added, we might want to revisit this
    static_assert(size_t(NozzleType::_cnt) == 2);
    switch (response) {
    case Response::PrusaStock:
        rResult.nozzle_type = NozzleType::Normal;
        return LoopResult::RunNext;
    case Response::HighFlow:
        rResult.nozzle_type = NozzleType::HighFlow;
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_HotEndSock::stateAskRetryInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::SpecifyHotEnd_retry);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_HotEndSock::stateAskRetry() {
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
