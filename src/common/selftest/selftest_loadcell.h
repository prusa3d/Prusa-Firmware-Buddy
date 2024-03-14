/**
 * @file selftest_loadcell.h
 * @author Radek Vana
 * @brief part of selftest testing loadcell
 * @date 2021-09-29
 */
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "algorithm_range.hpp"
#include "selftest_loadcell_config.hpp"
#include "sensor_data_buffer.hpp"
#include "selftest_log.hpp"

namespace selftest {

class CSelftestPart_Loadcell {
    IPartHandler &rStateMachine;
    const LoadcellConfig_t &rConfig;
    SelftestLoadcell_t &rResult;
    float currentZ;
    float targetZ;
    float begin_target_temp;
    uint32_t time_start;
    uint32_t time_start_countdown;
    uint32_t time_start_tap;
    uint8_t last_shown_countdown_number;
    bool need_cooling;
    LogTimer log;
    LogTimer log_fast;

public:
    CSelftestPart_Loadcell(IPartHandler &state_machine, const LoadcellConfig_t &config,
        SelftestLoadcell_t &result);
    ~CSelftestPart_Loadcell();

    LoopResult stateMoveUpInit();
    LoopResult stateMoveUp();
    LoopResult stateMoveUpWaitFinish();
    LoopResult stateToolSelectInit();
    LoopResult stateToolSelectWaitFinish();
    LoopResult stateConnectionCheck();
    LoopResult stateCooldownInit();
    LoopResult stateCooldown();
    LoopResult stateCooldownDeinit();
    LoopResult stateCycleMark() { return LoopResult::MarkLoop0; }
    LoopResult stateAskAbortInit();
    LoopResult stateAskAbort();
    LoopResult stateTapCheckCountDownInit();
    LoopResult stateTapCheckCountDown();
    LoopResult stateTapCheckInit();
    LoopResult stateTapCheck();
    LoopResult stateTapOk();
};

}; // namespace selftest
