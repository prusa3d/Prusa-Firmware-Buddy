// selftest_heater.cpp

#include "selftest_heater.h"
#include "hwio.h"
#include "wizard_config.hpp"
#include "../../Marlin/src/module/temperature.h"

#define TEMP_DIFF_LIMIT          0.25F
#define TEMP_DELTA_LIMIT         0.05F
#define TEMP_MEASURE_CYCLE_DELAY 1000
#define TEMP_MEASURE_TIME        40000
#define TEMP_WAIT_CYCLE_DELAY    2000

CSelftestPart_Heater::CSelftestPart_Heater(const selftest_heater_config_t *pconfig)
    : m_State(spsStart)
    , m_pConfig(pconfig) {
}

bool CSelftestPart_Heater::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State != spsFinished) && (m_State != spsAborted));
}

bool CSelftestPart_Heater::Start() {
    if (IsInProgress())
        return false;
    m_State = spsStart;
    return true;
}

bool CSelftestPart_Heater::Loop() {
    switch (m_State) {
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
        setTargetTemp(m_pConfig->max_temp);
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
        if ((m_Time - m_MeasureStartTime) < TEMP_MEASURE_TIME) {
            m_Time = Selftest.m_Time;
            m_Temp = 0;
            m_TempCount = 0;
            return true;
        }
        break;
    case spsFinish:
        setTargetTemp(0);
        Selftest.log_printf("%s Finished\n", m_pConfig->partname);
        break;
    case spsFinished:
    case spsAborted:
        return false;
    }
    return next();
}

bool CSelftestPart_Heater::Abort() {
    return true;
}

uint8_t CSelftestPart_Heater::getFSMState_cool() {
    if (m_State < spsWait)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State == spsWait)
        return (uint8_t)(SelftestSubtestState_t::running);
    else
        return (uint8_t)(SelftestSubtestState_t::ok);
}

uint8_t CSelftestPart_Heater::getFSMState_heat() {
    if (m_State < spsMeasure)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State == spsMeasure)
        return (uint8_t)(SelftestSubtestState_t::running);
    else
        return (uint8_t)(SelftestSubtestState_t::ok);
}

bool CSelftestPart_Heater::next() {
    if ((m_State == spsFinished) || (m_State == spsAborted))
        return false;
    m_State = (TestState)((int)m_State + 1);
    return ((m_State != spsFinished) && (m_State != spsAborted));
}

uint32_t CSelftestPart_Heater::estimate(const selftest_heater_config_t *pconfig) {
    uint32_t total_time = 60000;
    return total_time;
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
