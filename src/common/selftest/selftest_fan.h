// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"
#include "fanctl.h"

struct selftest_fan_config_t {
    const char *partname;
    CFanCtl &fanctl;
    int pwm_start;
    int pwm_step;
    const uint16_t *rpm_min_table;
    const uint16_t *rpm_max_table;
    uint8_t steps;
};

class CSelftestPart_Fan : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle = 0,
        spsStart,
        spsWait_stopped,
        spsWait_rpm,
        spsMeasure_rpm,
        spsFinish,
        spsFinished,
        spsAborted,
        spsFailed,
    };

public:
    CSelftestPart_Fan(const selftest_fan_config_t &config);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

public:
    uint8_t getFSMState();

protected:
    static uint32_t estimate(const selftest_fan_config_t &config);
    void restorePWM();

protected:
    const selftest_fan_config_t &m_config;
    uint32_t m_Time;
    uint8_t m_Step;
    uint8_t initial_pwm;
    uint16_t m_SampleCount;
    uint32_t m_SampleSum;
};
