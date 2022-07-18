// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "selftest_fan_config.hpp"

namespace selftest {

class CSelftestPart_Fan {
    IPartHandler &state_machine;
    const FanConfig_t &m_config;
    SelftestFan_t &rResult;
    SelftestFan_t &rLastResult;
    uint32_t m_StartTime;
    uint32_t m_EndTime;
    uint32_t m_SampleSum;
    uint16_t m_SampleCount;
    uint8_t m_Step;

    static uint32_t estimate(const FanConfig_t &config);
    void actualizeProgress() const;

public:
    CSelftestPart_Fan(IPartHandler &state_machine, const FanConfig_t &config,
        SelftestFan_t &result, SelftestFan_t &lastResult);
    ~CSelftestPart_Fan();

    LoopResult stateStart();
    LoopResult stateWaitStopped();
    LoopResult stateCycleMark() { return LoopResult::MarkLoop; }
    LoopResult stateWaitRpm();
    LoopResult stateMeasureRpm();
};

};
