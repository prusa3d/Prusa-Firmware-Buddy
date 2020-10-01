// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"

typedef struct _selftest_heater_config_t {
    const char *partname;
    uint8_t heater;
    int16_t start_temp;
    int16_t max_temp;
} selftest_heater_config_t;

class CSelftestPart_Heater : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle,
        spsStart,
        spsWait,
        spsMeasure,
        spsFinish,
        spsFinished,
        spsAborted,
    };

public:
    CSelftestPart_Heater(const selftest_heater_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

protected:
    bool next();
    static uint32_t estimate(const selftest_heater_config_t *pconfig);

protected:
    float getTemp();
    void setTargetTemp(int target_temp);

protected:
    TestState m_State;
    const selftest_heater_config_t *m_pConfig;
    uint32_t m_Time;
    uint32_t m_MeasureStartTime;
    float m_Temp;
    float m_TempDiffSum;
    float m_TempDeltaSum;
    uint16_t m_TempCount;
};
