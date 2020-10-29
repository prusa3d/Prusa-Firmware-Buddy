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
    const uint16_t *rpm_min_table;
    const uint16_t *rpm_max_table;
    uint8_t steps;
} selftest_fan_config_t;

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
    CSelftestPart_Fan(const selftest_fan_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

public:
    uint8_t getFSMState();

protected:
    static uint32_t estimate(const selftest_fan_config_t *pconfig);

protected:
    const selftest_fan_config_t *m_pConfig;
    uint32_t m_Time;
    uint8_t m_Step;
    uint16_t m_SampleCount;
    uint32_t m_SampleSum;
};
