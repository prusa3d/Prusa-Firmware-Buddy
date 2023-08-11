/**
 * @file selftest_hot_end_sock.hpp
 */
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_hot_end_sock_config.hpp"

namespace selftest {

/**
 * @brief Object handling states of hot end configuration
 */
class CSelftestPart_HotEndSock {

    IPartHandler &rStateMachine;
    const HotEndSockConfig &rConfig;
    SelftestHotEndSockType &rResult;

public:
    CSelftestPart_HotEndSock(IPartHandler &state_machine, const HotEndSockConfig &config,
        SelftestHotEndSockType &result);
    LoopResult stateStart();
    LoopResult stateAskAdjust();
    LoopResult stateAskSockInit();
    LoopResult stateAskSock();
    LoopResult stateAskNozzleInit();
    LoopResult stateAskNozzle();
    LoopResult stateAskRetryInit();
    LoopResult stateAskRetry();
};

}; // namespace selftest
