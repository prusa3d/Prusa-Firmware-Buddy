// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"
#include "fanctl.h"

typedef struct _selftest_heater_config_t {
    const char *partname;
    uint32_t heat_time_ms;
    int16_t start_temp;
    int16_t undercool_temp;
    int16_t target_temp;
    int16_t heat_min_temp;
    int16_t heat_max_temp;
    uint8_t heater;
} selftest_heater_config_t;

class CSelftestPart_Heater : public CSelftestPart {
public:
    enum TestState : uint8_t {
        spsIdle = 0,
        spsStart, //child will use fans
        spsCooldown,
        spsSetTargetTemp,
        spsWait,
        spsMeasure,
        spsFinish, //child will restore fans
        spsFinished,
        spsAborted,
        spsFailed,
    };

public:
    CSelftestPart_Heater(const selftest_heater_config_t *pconfig);

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;

public:
    uint8_t getFSMState_cool();
    uint8_t getFSMState_heat();

protected:
    static uint32_t estimate(const selftest_heater_config_t *pconfig);

protected:
    float getTemp();
    void setTargetTemp(int target_temp);

protected:
    const selftest_heater_config_t *m_pConfig;
    uint32_t m_Time;
    uint32_t m_MeasureStartTime;
    float m_Temp;
    float m_TempDiffSum;
    float m_TempDeltaSum;
    uint16_t m_TempCount;
    bool need_cooldown;

public:
    virtual void stateStart();
};

//extra fan control
class CSelftestPart_HeaterHotend : public CSelftestPart_Heater {
    const CFanCtl *m_pConfig_fan0;
    const CFanCtl *m_pConfig_fan1;
    uint8_t fan0_initial_pwm;
    uint8_t fan1_initial_pwm;

public:
    CSelftestPart_HeaterHotend(const selftest_heater_config_t *pconfig, const CFanCtl *pfanctl0, const CFanCtl *pfanctl1);
};
