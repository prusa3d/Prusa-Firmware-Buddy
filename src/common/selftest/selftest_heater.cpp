// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "wizard_config.hpp"
#include "../../Marlin/src/module/temperature.h"

#define TEMP_DIFF_LIMIT          0.25F
#define TEMP_DELTA_LIMIT         0.05F
#define TEMP_MEASURE_CYCLE_DELAY 1000
#define TEMP_WAIT_CYCLE_DELAY    2000

CSelftestPart_Heater::CSelftestPart_Heater(const selftest_heater_config_t *pconfig)
    : m_pConfig(pconfig) {
    m_State = spsStart;
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

bool CSelftestPart_Heater::Loop() {
    switch ((TestState)m_State) {
    case spsIdle:
        return false;
    case spsStart:
        Selftest.log_printf("%s Started\n", m_pConfig->partname);
        m_Time = Selftest.m_Time;
        m_StartTime = m_Time;
        m_EndTime = m_StartTime + estimate(m_pConfig);
        hwio_fan_control_enable();
        m_TempDiffSum = 0;
        m_TempDiffSum = 0;
        m_TempCount = 0;
        m_Temp = getTemp();
        setTargetTemp(m_pConfig->start_temp);
        break;
    case spsWait: {
        if ((Selftest.m_Time - m_Time) < TEMP_WAIT_CYCLE_DELAY) {
            float temp = getTemp();
            float temp_diff = (temp - m_pConfig->start_temp);
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
        setTargetTemp(m_pConfig->target_temp);
        m_Time = Selftest.m_Time;
        m_MeasureStartTime = m_Time;
        m_Temp = 0;
        m_TempCount = 0;
        break;
    }
    case spsMeasure:
        if ((Selftest.m_Time - m_Time) < TEMP_MEASURE_CYCLE_DELAY) {
            float temp = getTemp();
            m_Temp += temp;
            m_TempCount++;
            return true;
        }
        m_Temp /= m_TempCount;
        Selftest.log_printf("%s %5u ms  %5.1f C\n", m_pConfig->partname, m_Time - m_MeasureStartTime, (double)m_Temp);
        if ((m_Time - m_MeasureStartTime) < m_pConfig->heat_time_ms) {
            m_Time = Selftest.m_Time;
            m_Temp = 0;
            m_TempCount = 0;
            return true;
        }
        if ((m_Temp < m_pConfig->heat_min_temp) || (m_Temp > m_pConfig->heat_max_temp)) {
            Selftest.log_printf("%s %i out of range (%i - %i)\n", m_pConfig->partname, (int)m_Temp, m_pConfig->heat_min_temp, m_pConfig->heat_max_temp);
            m_Result = sprFailed;
            m_State = spsFinish;
            return true;
        }
        break;
    case spsFinish:
        setTargetTemp(0);
        if (m_Result == sprFailed)
            m_State = spsFailed;
        else
            m_Result = sprPassed;
        Selftest.log_printf("%s %s\n", m_pConfig->partname, (m_Result == sprPassed) ? "Passed" : "Failed");
        break;
    case spsFinished:
    case spsAborted:
    case spsFailed:
        return false;
    }
    return next();
}

bool CSelftestPart_Heater::Abort() {
    return true;
}

uint8_t CSelftestPart_Heater::getFSMState_cool() {
    if (m_State <= spsStart)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State == spsWait)
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

uint32_t CSelftestPart_Heater::estimate(const selftest_heater_config_t *pconfig) {
    return pconfig->heat_time_ms;
}

float CSelftestPart_Heater::getTemp() {
    if (m_pConfig->heater == 0xff)
        return thermalManager.temp_bed.celsius;
    return thermalManager.temp_hotend[m_pConfig->heater].celsius;
}

void CSelftestPart_Heater::setTargetTemp(int target_temp) {
    if (m_pConfig->heater == 0xff)
        thermalManager.setTargetBed(target_temp);
    else
        thermalManager.setTargetHotend(target_temp, m_pConfig->heater);
}
