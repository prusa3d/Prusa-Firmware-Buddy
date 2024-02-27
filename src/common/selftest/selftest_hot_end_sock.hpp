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
class CSelftestPart_HotendSpecify {
    IPartHandler &rStateMachine;
    const HotendSpecifyConfig &rConfig;
    SelftestHotendSpecifyType &rResult;

public:
    CSelftestPart_HotendSpecify(IPartHandler &state_machine, const HotendSpecifyConfig &config, SelftestHotendSpecifyType &result);

    LoopResult stateStart();
    LoopResult stateAskAdjust();
    LoopResult stateAskHotendInit();
    LoopResult stateAskHotend();
    LoopResult stateAskNozzleInit();
    LoopResult stateAskNozzle();
    LoopResult stateAskRetryInit();
    LoopResult stateAskRetry();
};

}; // namespace selftest
