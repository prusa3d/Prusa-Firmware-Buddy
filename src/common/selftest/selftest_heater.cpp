// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "wizard_config.hpp"
#include "selftest_log.hpp"
#include "fanctl.h"
#include "../../Marlin/src/module/temperature.h"
#include "i_selftest.hpp"
#include "algorithm_scale.hpp"

using namespace selftest;

static constexpr float TEMP_DIFF_LIMIT = 0.25;
static constexpr float TEMP_DELTA_LIMIT = 0.05F;
static constexpr uint32_t TEMP_MEASURE_CYCLE_DELAY = 1000;
static constexpr uint32_t TEMP_WAIT_CYCLE_DELAY = 2000;

CSelftestPart_Heater::CSelftestPart_Heater(IPartHandler &state_machine, const HeaterConfig_t &config,
    SelftestHeater_t &result)
    : state_machine(state_machine)
    , m_config(config)
    , rResult(result)
    , storedKp(config.refKp)
    , storedKi(config.refKi)
    , storedKd(config.refKd)
    , last_progress(0)
    , log(2000) {

    //looked into marlin and it seems all PID values are used as numerator
    //switch regulator into on/off mode
    config.refKp = 1000000;
    config.refKi = 0;
    config.refKd = 0;
    thermalManager.updatePID();
    LogInfo("%s Started");
    LogInfo("target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
    LogInfo("%s heater PID regulator changed to P regulator", m_config.partname);
}

CSelftestPart_Heater::~CSelftestPart_Heater() {
    LogInfo("%s finish, target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
    m_config.setTargetTemp(0);

    m_config.refKp = storedKp;
    m_config.refKi = storedKi;
    m_config.refKd = storedKd;
    thermalManager.updatePID();
    LogInfo("%s heater PID regulator restored", m_config.partname);
}

uint32_t CSelftestPart_Heater::estimate(const HeaterConfig_t &config) {
    return config.heat_time_ms;
}

LoopResult CSelftestPart_Heater::stateStart() {
    m_StartTime = SelftestInstance().GetTime();
    m_EndTime = m_StartTime + estimate(m_config);
    //m_TempDiffSum = 0;
    //m_TempDiffSum = 0;
    //m_TempCount = 0;
    begin_temp = m_config.getTemp();
    enable_cooldown = m_config.getTemp() >= m_config.start_temp;
    m_config.setTargetTemp(0);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTakeControlOverFans() {
    LogInfo("%s took control of fans", m_config.partname);
    m_config.print_fan.EnterSelftestMode();
    m_config.heatbreak_fan.EnterSelftestMode();
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansActivate() {
    if (enable_cooldown) {
        LogInfo("%s set fans to maximum", m_config.partname);
        m_config.print_fan.SelftestSetPWM(255);     //it will be restored by ExitSelftestMode
        m_config.heatbreak_fan.SelftestSetPWM(255); //it will be restored by ExitSelftestMode
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateCooldownInit() {
    if (enable_cooldown) {
        rResult.prep_state = SelftestSubtestState_t::running;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateCooldown() {
    if (!enable_cooldown) {
        LogInfo("%s cooldown not needed, target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
        return LoopResult::RunNext;
    }

    LogInfoTimed(log, "%s cooling down, target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());

    if (m_config.getTemp() > m_config.undercool_temp) {
        //m_config.undercool_temp .. 100%
        //begin_temp              ..   0%
        actualizeProgress(m_config.getTemp(), begin_temp, m_config.undercool_temp);
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateFansDeactivate() {
    m_config.print_fan.ExitSelftestMode();
    m_config.heatbreak_fan.ExitSelftestMode();
    LogInfo("%s returned control of fans", m_config.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateTargetTemp() {
    LogInfo("%s set target, target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
    rResult.prep_state = SelftestSubtestState_t::running; // waiting for preheat temperature
    m_config.setTargetTemp(m_config.target_temp);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_Heater::stateWait() {
    float current_temp = m_config.getTemp();
    if (current_temp >= m_config.start_temp) {
        rResult.prep_state = SelftestSubtestState_t::ok;      // preheat temperature ok
        rResult.heat_state = SelftestSubtestState_t::running; // waiting final heat
        m_MeasureStartTime = SelftestInstance().GetTime();
        m_StartTime = SelftestInstance().GetTime();
        m_EndTime = m_StartTime + estimate(m_config);
        rResult.progress = 0;
        LogInfo("%s wait start temp reached: target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
        return LoopResult::RunNext;
    }
    LogInfoTimed(log, "%s wait, run: target: %d current: %f", m_config.partname, m_config.target_temp, (double)current_temp);

    //m_config.start_temp     .. 100%
    //m_config.undercool_temp ..   0%
    actualizeProgress(current_temp, m_config.undercool_temp, m_config.start_temp);
    return LoopResult::RunCurrent;

#if (0)
    //used to be commented code I just moved it and wrapped in #if (0) instead
    if ((Selftest.m_Time - m_Time) < TEMP_WAIT_CYCLE_DELAY) {
        float temp = m_config.getTemp();
        float temp_diff = (temp - m_config.start_temp);
        float temp_delta = (temp - m_Temp);
        m_Temp = temp;
        m_TempDiffSum += temp_diff;
        m_TempDeltaSum += temp_delta;
        m_TempCount++;
        return true;
    }
    m_TempDiffSum /= m_TempCount;
    m_TempDeltaSum /= m_TempCount;
    if ((fabsf(m_TempDiffSum) > TEMP_DIFF_LIMIT) || (fabsf(m_TempDeltaSum) > TEMP_DELTA_LIMIT)) {
        m_Time = Selftest.m_Time;
        m_TempDiffSum = 0;
        m_TempDeltaSum = 0;
        m_TempCount = 0;
        return true;
    }
    setTargetTemp(m_config.target_temp);
    m_Time = Selftest.m_Time;
    m_MeasureStartTime = m_Time;
    m_Temp = 0;
    m_TempCount = 0;
    break;
#endif // 0
}

LoopResult CSelftestPart_Heater::stateMeasure() {
    if (int(m_EndTime - SelftestInstance().GetTime()) > 0) {
        //time based progress
        actualizeProgress(SelftestInstance().GetTime(), m_StartTime, m_EndTime);
        return LoopResult::RunCurrent;
    }
#if (0)
    //used to be commented code I just moved it and wrapped in #if (0) instead
    if ((Selftest.m_Time - m_Time) < TEMP_MEASURE_CYCLE_DELAY) {
        float temp = m_config.getTemp();
        m_Temp += temp;
        m_TempCount++;
        return true;
    }
    m_Temp /= m_TempCount;
    Selftest.log_printf("%s %5u ms  %5.1f C\n", m_config.partname, m_Time - m_MeasureStartTime, (double)m_Temp);
    if ((m_Time - m_MeasureStartTime) < m_config.heat_time_ms) {
        m_Time = Selftest.m_Time;
        m_Temp = 0;
        m_TempCount = 0;
        return true;
    }
#endif // 0

    if ((m_config.getTemp() < m_config.heat_min_temp) || (m_config.getTemp() > m_config.heat_max_temp)) {
        LogError("%s %i out of range (%i - %i)\n", m_config.partname, (int)m_config.getTemp(), m_config.heat_min_temp, m_config.heat_max_temp);
        return LoopResult::Fail;
    }
    LogInfo("%s measure, target: %d current: %f", m_config.partname, m_config.target_temp, (double)m_config.getTemp());
    return LoopResult::RunNext;
}

void CSelftestPart_Heater::actualizeProgress(float current, float progres_start, float progres_end) const {
    if (progres_start >= progres_end)
        return; // don't have estimated end set correctly
    uint8_t current_progress = scale_percent_avoid_overflow(current, progres_start, progres_end);
    rResult.progress = std::max(rResult.progress, current_progress); // heater progress can only rise
}
