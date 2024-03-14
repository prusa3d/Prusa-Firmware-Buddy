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
    rResult.has_sock = config_store().nozzle_sock.get();
    rResult.prusa_stock_nozzle = config_store().nozzle_type.get() == 0;
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
        rResult.has_sock = true;
        return LoopResult::RunNext;
    case Response::No:
        rResult.has_sock = false;
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
    switch (response) {
    case Response::PrusaStock:
        rResult.prusa_stock_nozzle = true;
        return LoopResult::RunNext;
    case Response::HighFlow:
        rResult.prusa_stock_nozzle = false;
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
