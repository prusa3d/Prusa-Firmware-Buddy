// selftest_fan.cpp

#include "selftest_fan.h"
#include "fanctl.h"

#define FANTEST_STOP_DELAY    2000
#define FANTEST_WAIT_DELAY    2500
#define FANTEST_MEASURE_DELAY 2500

CSelftestPart_Fan::CSelftestPart_Fan(const selftest_fan_config_t *pconfig)
    : m_State(spsStart)
    , m_pConfig(pconfig) {
}

bool CSelftestPart_Fan::IsInProgress() const {
    return ((m_State != spsIdle) && (m_State != spsFinished) && (m_State != spsAborted));
}

bool CSelftestPart_Fan::Start() {
    if (IsInProgress())
        return false;
    m_State = spsStart;
    return true;
}

bool CSelftestPart_Fan::Loop() {
    switch (m_State) {
    case spsIdle:
        return false;
    case spsStart:
        Selftest.log_printf("%s Started\n", m_pConfig->partname);
        m_Time = Selftest.m_Time;
        m_StartTime = m_Time;
        m_EndTime = m_StartTime + estimate(m_pConfig);
        if ((m_pConfig->pfanctl->getPWM() == 0) && (m_pConfig->pfanctl->getActualRPM() == 0)) {
            m_Time += FANTEST_STOP_DELAY;
            m_EndTime -= FANTEST_STOP_DELAY;
        }
        m_pConfig->pfanctl->setPWM(0);
        break;
    case spsWait_stopped:
        if ((Selftest.m_Time - m_Time) <= FANTEST_STOP_DELAY)
            return true;
        m_pConfig->pfanctl->setPWM(m_pConfig->pwm_start);
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
    case spsMeasure_rpm:
        if ((Selftest.m_Time - m_Time) <= FANTEST_MEASURE_DELAY) {
            m_SampleCount++;
            m_SampleSum += m_pConfig->pfanctl->getActualRPM();
            return true;
        }
        Selftest.log_printf("%s at %u%% PWM = %u RPM\n", m_pConfig->partname, m_pConfig->pfanctl->getPWM(), m_SampleSum / m_SampleCount);
        if (++m_Step < m_pConfig->steps) {
            m_pConfig->pfanctl->setPWM(m_pConfig->pfanctl->getPWM() + m_pConfig->pwm_step);
            m_Time = Selftest.m_Time;
            m_State = spsWait_rpm;
            return true;
        }
        break;
    case spsFinish:
        m_pConfig->pfanctl->setPWM(0);
        Selftest.log_printf("%s Finished\n", m_pConfig->partname);
        break;
    case spsFinished:
    case spsAborted:
        return false;
    }
    return next();
}

bool CSelftestPart_Fan::Abort() {
    if (!IsInProgress())
        return false;
    if (m_pConfig->pfanctl)
        m_pConfig->pfanctl->setPWM(0);
    m_State = spsAborted;
    return true;
}

bool CSelftestPart_Fan::next() {
    if ((m_State == spsFinished) || (m_State == spsAborted))
        return false;
    m_State = (TestState)((int)m_State + 1);
    return ((m_State != spsFinished) && (m_State != spsAborted));
}

uint32_t CSelftestPart_Fan::estimate(const selftest_fan_config_t *pconfig) {
    uint32_t total_time = FANTEST_STOP_DELAY + pconfig->steps * (FANTEST_WAIT_DELAY + FANTEST_MEASURE_DELAY);
    return total_time;
}
