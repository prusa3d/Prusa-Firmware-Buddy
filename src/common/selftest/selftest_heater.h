// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"
#include "fanctl.h"
#include "../../Marlin/src/module/temperature.h"

struct selftest_heater_config_t {
    const char *partname;
    uint32_t heat_time_ms;
    int16_t start_temp;
    int16_t undercool_temp;
    int16_t target_temp;
    int16_t heat_min_temp;
    int16_t heat_max_temp;
    uint8_t heater;
};

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
    CSelftestPart_Heater(const selftest_heater_config_t &config, PID_t &pid);
    CSelftestPart_Heater(const selftest_heater_config_t &config, PIDC_t &pid); // marlin uses PIDC_t in nozzle
    CSelftestPart_Heater(const selftest_heater_config_t &config, float &kp, float &ki, float &kd);
    virtual ~CSelftestPart_Heater();

public:
    virtual bool IsInProgress() const override;

public:
    virtual bool Start() override;
    virtual bool Loop() override;
    virtual bool Abort() override;
    virtual float GetProgress() override;

public:
    uint8_t getFSMState_prepare();
    uint8_t getFSMState_heat();

protected:
    static uint32_t estimate(const selftest_heater_config_t &config);

protected:
    float getTemp();
    void setTargetTemp(int target_temp);

protected:
    const selftest_heater_config_t &m_config;
    float &refKp;
    float &refKi;
    float &refKd;
    float storedKp;
    float storedKi;
    float storedKd;
    uint32_t m_Time;
    uint32_t m_MeasureStartTime;
    float begin_temp;
    float m_Temp;        //actual temp?
    float last_progress; //cannot go backwards
    //float m_TempDiffSum;
    //float m_TempDeltaSum;
    //uint16_t m_TempCount;
    bool enable_cooldown;
    static bool can_enable_fan_control;

    virtual void stateStart();
    virtual void stateTargetTemp();
};

//extra fan control
class CSelftestPart_HeaterHotend : public CSelftestPart_Heater {
    CFanCtl &m_fanCtlPrint;
    CFanCtl &m_fanCtlHeatBreak;
    uint8_t print_fan_initial_pwm;
    uint8_t heatbreak_fan_initial_pwm;
    bool stored_can_enable_fan_control;

protected:
    virtual void stateStart() override;
    virtual void stateTargetTemp() override;

public:
    CSelftestPart_HeaterHotend(const selftest_heater_config_t &config, PID_t &pid, CFanCtl &fanCtlPrint, CFanCtl &fanCtlHeatBreak);
    CSelftestPart_HeaterHotend(const selftest_heater_config_t &config, PIDC_t &pid, CFanCtl &fanCtlPrint, CFanCtl &fanCtlHeatBreak);
};
