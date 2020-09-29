// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"
#include "fanctl.h"

typedef struct _selftest_fan_config_t {
    const char *partname;
    CFanCtl *pfanctl;
    int pwm_start;
    int pwm_step;
    int pwm_steps;
} selftest_fan_config_t;

class CSelftestPart_Fan : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle,
        spsStart,
        spsWait_stopped,
        spsWait_rpm,
        spsMeasure_rpm,
        spsFinish,
        spsFinished,
        spsAborted,
    };

public:
    CSelftestPart_Fan(const selftest_fan_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

protected:
    bool next();
    static uint32_t estimate(const selftest_fan_config_t *pconfig);

protected:
    TestState m_State;
    const selftest_fan_config_t *m_pConfig;
    uint32_t m_Time;
    uint8_t m_Step;
    uint16_t m_SampleCount;
    uint32_t m_SampleSum;
};
