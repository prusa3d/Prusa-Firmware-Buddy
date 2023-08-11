// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_heaters_type.hpp"
#include "i_selftest_part.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_log.hpp"

namespace selftest {

class CSelftestPart_Heater {
public:
    IPartHandler &state_machine;
    const HeaterConfig_t &m_config;

private:
    SelftestHeater_t &rResult;
    float storedKp;
    float storedKi;
    float storedKd;
    uint32_t m_StartTime;
    uint32_t m_EndTime;
    uint32_t m_MeasureStartTime;
    float begin_temp;
    float last_progress; // cannot go backwards
    // float m_TempDiffSum;
    // float m_TempDeltaSum;
    // uint16_t m_TempCount;
    bool enable_cooldown;
    LogTimer log;

    static uint32_t estimate(const HeaterConfig_t &config);
    void actualizeProgress(float current, float progres_start, float progres_end) const;

public:
    CSelftestPart_Heater(IPartHandler &state_machine, const HeaterConfig_t &config,
        SelftestHeater_t &result);
    ~CSelftestPart_Heater();

    LoopResult stateStart();
    LoopResult stateTakeControlOverFans(); // also enters fan selftest mode
    LoopResult stateFansActivate();
    LoopResult stateCooldownInit();
    LoopResult stateCooldown();
    LoopResult stateFansDeactivate(); // also exits fan selftest mode
    LoopResult stateTargetTemp();
    LoopResult stateWait();
    LoopResult stateMeasure();
};

}; // namespace selftest
