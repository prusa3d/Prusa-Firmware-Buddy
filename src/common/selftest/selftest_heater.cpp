// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "wizard_config.hpp"

static constexpr float TEMP_DIFF_LIMIT = 0.25;
static constexpr float TEMP_DELTA_LIMIT = 0.05F;
static constexpr uint32_t TEMP_MEASURE_CYCLE_DELAY = 1000;
static constexpr uint32_t TEMP_WAIT_CYCLE_DELAY = 2000;

CSelftestPart_Heater::CSelftestPart_Heater(const selftest_heater_config_t &config, PID_t &pid)
    : CSelftestPart_Heater(config, pid.Kp, pid.Ki, pid.Kd) {}

CSelftestPart_Heater::CSelftestPart_Heater(const selftest_heater_config_t &config, PIDC_t &pid)
    : CSelftestPart_Heater(config, pid.Kp, pid.Ki, pid.Kd) {}

CSelftestPart_Heater::CSelftestPart_Heater(const selftest_heater_config_t &config, float &kp, float &ki, float &kd)
    : m_config(config)
    , refKp(kp)
    , refKi(ki)
    , refKd(kd)
    , storedKp(kp)
    , storedKi(ki)
    , storedKd(kd)
    , last_progress(0) {
    m_State = spsStart;
    //looked into marlin and it seems all PID values are used as numerator
    //switch regulator into on/off mode
    refKp = 1000000;
    refKi = 0;
    refKd = 0;
    thermalManager.updatePID();
}

CSelftestPart_Heater::~CSelftestPart_Heater() {
    refKp = storedKp;
    refKi = storedKi;
    refKd = storedKd;
    thermalManager.updatePID();
}

bool CSelftestPart_Heater::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State < spsFinished));
}

bool CSelftestPart_Heater::Start() {
    if (IsInProgress())
        return false;
    m_State = spsStart;
    return true;
}

void CSelftestPart_Heater::stateStart() {
    Selftest.log_printf("%s Started\n", m_config.partname);
    m_Time = Selftest.m_Time;
    m_StartTime = m_Time;
    m_EndTime = m_StartTime + estimate(m_config);
    //m_TempDiffSum = 0;
    //m_TempDiffSum = 0;
    //m_TempCount = 0;
    m_Temp = getTemp();
    begin_temp = m_Temp;
    enable_cooldown = m_Temp >= m_config.start_temp;
    setTargetTemp(0);
}

void CSelftestPart_Heater::stateTargetTemp() {
    if (can_enable_fan_control)
        hwio_fan_control_enable();
    setTargetTemp(m_config.target_temp);
}

float CSelftestPart_Heater::GetProgress() {
    float progress = 0;
    switch ((TestState)m_State) {
    case spsIdle:
    case spsStart:
    case spsSetTargetTemp:
        break;
    case spsCooldown:
        //m_Temp == m_config.undercool_temp .. 100%
        //m_Temp == begin_temp              ..   0%
        progress = 100.0F * (begin_temp - m_Temp) / (begin_temp - m_config.undercool_temp);
        if (progress < 0)
            progress = 0;
        if (progress > 100.0F)
            progress = 100.0F;
        break;
    case spsWait:
        //m_Temp == m_config.start_temp .. 100%
        //m_Temp == begin_temp          ..   0%
        progress = 100.0F * (m_Temp - begin_temp) / (m_config.start_temp - begin_temp);
        if (progress < 0)
            progress = 0;
        if (progress > 100.0F)
            progress = 100.0F;
        break;
    case spsMeasure:
        progress = CSelftestPart::GetProgress();
        break;
    case spsFinish:
    case spsFinished:
    case spsAborted:
    case spsFailed:
        progress = 100;
        break;
    }

    last_progress = last_progress < progress ? progress : last_progress;
    return last_progress;
}

bool CSelftestPart_Heater::Loop() {
    switch ((TestState)m_State) {
    case spsIdle:
        return false;
    case spsStart:
        stateStart();
        break;
    case spsCooldown:
        if (!enable_cooldown)
            break;
        m_Temp = getTemp();
        if (m_Temp > m_config.undercool_temp)
            return true;
        //new begin_temp, will start heating now
        begin_temp = m_Temp;
        break;
    case spsSetTargetTemp:
        stateTargetTemp();
        break;
    case spsWait: {
        m_Temp = getTemp();
        if (m_Temp >= m_config.start_temp) {
            m_Time = Selftest.m_Time;
            m_MeasureStartTime = m_Time;
            m_StartTime = m_Time;
            m_EndTime = m_StartTime + estimate(m_config);
            break;
        }
        return true;

        /*        if ((Selftest.m_Time - m_Time) < TEMP_WAIT_CYCLE_DELAY) {
            float temp = getTemp();
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
        break;*/
    }
    case spsMeasure:
        if (int(m_EndTime - Selftest.m_Time) > 0) {
            m_Temp = getTemp();
            return true;
        }

        /* if ((Selftest.m_Time - m_Time) < TEMP_MEASURE_CYCLE_DELAY) {
            float temp = getTemp();
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
        }*/
        if ((m_Temp < m_config.heat_min_temp) || (m_Temp > m_config.heat_max_temp)) {
            Selftest.log_printf("%s %i out of range (%i - %i)\n", m_config.partname, (int)m_Temp, m_config.heat_min_temp, m_config.heat_max_temp);
            m_Result = sprFailed;
        }
        break;
    case spsFinish:
        setTargetTemp(0);
        if (m_Result == sprFailed)
            m_State = spsFailed;
        else
            m_Result = sprPassed;
        Selftest.log_printf("%s %s\n", m_config.partname, (m_Result == sprPassed) ? "Passed" : "Failed");
        break;
    case spsFinished:
    case spsAborted:
    case spsFailed:
        return false;
    }
    last_progress = 0;
    return next();
}

bool CSelftestPart_Heater::Abort() {
    return true;
}

uint8_t CSelftestPart_Heater::getFSMState_prepare() {
    if (m_State <= spsStart)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State <= spsWait)
        return (uint8_t)(SelftestSubtestState_t::running);
    else
        return (uint8_t)(SelftestSubtestState_t::ok);
}

uint8_t CSelftestPart_Heater::getFSMState_heat() {
    if (m_State < spsMeasure)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State < spsFinished)
        return (uint8_t)(SelftestSubtestState_t::running);
    return (uint8_t)((m_Result == sprPassed) ? (SelftestSubtestState_t::ok) : (SelftestSubtestState_t::not_good));
}

uint32_t CSelftestPart_Heater::estimate(const selftest_heater_config_t &config) {
    return config.heat_time_ms;
}

float CSelftestPart_Heater::getTemp() {
    if (m_config.heater == 0xff)
        return thermalManager.temp_bed.celsius;
    return thermalManager.temp_hotend[m_config.heater].celsius;
}

void CSelftestPart_Heater::setTargetTemp(int target_temp) {
    if (m_config.heater == 0xff)
        thermalManager.setTargetBed(target_temp);
    else
        thermalManager.setTargetHotend(target_temp, m_config.heater);
}

bool CSelftestPart_Heater::can_enable_fan_control = true;

/*****************************************************************************/
//CSelftestPart_HeaterHotend

CSelftestPart_HeaterHotend::CSelftestPart_HeaterHotend(const selftest_heater_config_t &config, PID_t &pid, CFanCtl &fanCtlPrint, CFanCtl &fanCtlHeatBreak)
    : CSelftestPart_Heater(config, pid)
    , m_fanCtlPrint(fanCtlPrint)
    , m_fanCtlHeatBreak(fanCtlHeatBreak)
    , stored_can_enable_fan_control(can_enable_fan_control) {
    can_enable_fan_control = false;
}

CSelftestPart_HeaterHotend::CSelftestPart_HeaterHotend(const selftest_heater_config_t &config, PIDC_t &pid, CFanCtl &fanCtlPrint, CFanCtl &fanCtlHeatBreak)
    : CSelftestPart_Heater(config, pid)
    , m_fanCtlPrint(fanCtlPrint)
    , m_fanCtlHeatBreak(fanCtlHeatBreak)
    , stored_can_enable_fan_control(can_enable_fan_control) {
    can_enable_fan_control = false;
}

void CSelftestPart_HeaterHotend::stateStart() {
    CSelftestPart_Heater::stateStart();
    if (enable_cooldown) {
        hwio_fan_control_disable();
        m_fanCtlPrint.setPWM(255);     //marlin will restore it automatically
        m_fanCtlHeatBreak.setPWM(255); //marlin will restore it automatically
    }
}

void CSelftestPart_HeaterHotend::stateTargetTemp() {
    hwio_fan_control_enable(); //do not check can_enable_fan_control
    setTargetTemp(m_config.target_temp);
}
