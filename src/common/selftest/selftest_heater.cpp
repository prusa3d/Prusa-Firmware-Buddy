// selftest_heater.cpp

#include "selftest_heater.h"

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
        break;
    case spsWait:
        if ((Selftest.m_Time - m_Time) <= 1000)
            return true;
        break;
    case spsFinish:
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

bool CSelftestPart_Heater::next() {
    return true;
}

uint32_t CSelftestPart_Heater::estimate(const selftest_heater_config_t *pconfig) {
    uint32_t total_time = 1000;
    return total_time;
}
