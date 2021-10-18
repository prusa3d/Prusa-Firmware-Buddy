// selftest_fan.cpp

#include "selftest_fan.h"
#include "wizard_config.hpp"
#include "fanctl.h"
#include "config_features.h" //EXTRUDER_AUTO_FAN_TEMPERATURE
#include "marlin_server.h"   //marlin_server_get_temp_nozzle()

#define FANTEST_STOP_DELAY    2000
#define FANTEST_WAIT_DELAY    2500
#define FANTEST_MEASURE_DELAY 7500

CSelftestPart_Fan::CSelftestPart_Fan(const selftest_fan_config_t &config)
    : m_config(config) {
    m_State = spsStart;
}

bool CSelftestPart_Fan::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State < spsFinished));
}

bool CSelftestPart_Fan::Start() {
    if (IsInProgress())
        return false;
    m_State = spsStart;
    initial_pwm = m_config.fanctl.getPWM();
    return true;
}

bool CSelftestPart_Fan::Loop() {
    switch ((TestState)m_State) {
    case spsIdle:
        return false;
    case spsStart:
        Selftest.log_printf("%s Started\n", m_config.partname);
        m_Time = Selftest.m_Time;
        m_StartTime = m_Time;
        m_EndTime = m_StartTime + estimate(m_config);
        if ((m_config.fanctl.getPWM() == 0) && (m_config.fanctl.getActualRPM() == 0)) {
            m_Time += FANTEST_STOP_DELAY;
            m_EndTime -= FANTEST_STOP_DELAY;
        }
        m_config.fanctl.setPWM(0);
        break;
    case spsWait_stopped:
        if ((Selftest.m_Time - m_Time) <= FANTEST_STOP_DELAY)
            return true;
        m_config.fanctl.setPWM(m_config.pwm_start);
        m_Step = 0;
        m_Time = Selftest.m_Time;
        break;
    case spsWait_rpm:
        if ((Selftest.m_Time - m_Time) <= FANTEST_WAIT_DELAY)
            return true;
        m_Time = Selftest.m_Time;
        m_SampleCount = 0;
        m_SampleSum = 0;
        break;
    case spsMeasure_rpm: {
        if ((Selftest.m_Time - m_Time) <= FANTEST_MEASURE_DELAY) {
            m_SampleCount++;
            m_SampleSum += m_config.fanctl.getActualRPM();
            return true;
        }
        uint16_t rpm = m_SampleSum / m_SampleCount;
        Selftest.log_printf("%s at %u%% PWM = %u RPM\n", m_config.partname, 2 * (m_config.fanctl.getPWM()), rpm);
        if ((m_config.rpm_min_table != nullptr) && (m_config.rpm_max_table != nullptr))
            if ((rpm < m_config.rpm_min_table[m_Step]) || (rpm > m_config.rpm_max_table[m_Step])) {
                Selftest.log_printf("%s %u RPM out of range (%u - %u)\n", m_config.partname, rpm, m_config.rpm_min_table[m_Step], m_config.rpm_max_table[m_Step]);
                m_Result = sprFailed;
                m_State = spsFinish;
                return true;
            }
        if (++m_Step < m_config.steps) {
            m_config.fanctl.setPWM(m_config.fanctl.getPWM() + m_config.pwm_step);
            m_Time = Selftest.m_Time;
            m_State = spsWait_rpm;
            return true;
        }
        break;
    }
    case spsFinish:
        restorePWM();
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
    return next();
}

bool CSelftestPart_Fan::Abort() {
    if (!IsInProgress())
        return false;
    restorePWM();
    m_State = spsAborted;
    return true;
}

void CSelftestPart_Fan::restorePWM() {
    uint8_t pwm_to_restore;
    if (m_config.fanctl.isAutoFan())
        pwm_to_restore = (marlin_server_get_temp_nozzle() >= EXTRUDER_AUTO_FAN_TEMPERATURE) ? initial_pwm : 0;
    else
        pwm_to_restore = initial_pwm;

    m_config.fanctl.setPWM(pwm_to_restore);
}

uint8_t CSelftestPart_Fan::getFSMState() {
    if (m_State <= spsStart)
        return (uint8_t)(SelftestSubtestState_t::undef);
    else if (m_State < spsFinished)
        return (uint8_t)(SelftestSubtestState_t::running);
    return (uint8_t)((m_Result == sprPassed) ? (SelftestSubtestState_t::ok) : (SelftestSubtestState_t::not_good));
}

uint32_t CSelftestPart_Fan::estimate(const selftest_fan_config_t &config) {
    uint32_t total_time = FANTEST_STOP_DELAY + config.steps * (FANTEST_WAIT_DELAY + FANTEST_MEASURE_DELAY);
    return total_time;
}
