// selftest_axis.cpp

#include "selftest_axis.h"
#include "../../Marlin/src/module/stepper.h"

CSelftestPart_Axis::CSelftestPart_Axis(const selftest_axis_config_t *pconfig)
    : m_State(spsStart)
    , m_pConfig(pconfig) {
}

bool CSelftestPart_Axis::Start() {
    return true;
}

bool CSelftestPart_Axis::IsInProgress() const {
    return false;
}

bool CSelftestPart_Axis::Loop() {
    switch (m_State) {
    case spsIdle:
        return false;
    case spsStart:
        Selftest.log_printf("%s Started\n", m_pConfig->partname);
        m_Time = Selftest.m_Time;
        m_StartTime = m_Time;
        m_EndTime = m_StartTime + estimate(m_pConfig);
        if (m_Home == hsNone) {
            m_Home = hsHommingInProgress;
            //            homeaxis(X_AXIS);
            m_Home = hsHommingFinished;
        }
        break;
    case spsWaitHome:
        if ((Selftest.m_Time - m_Time) <= 1000)
            //        if (m_Home != hsHommingFinished)
            return true;
        m_Time = Selftest.m_Time;
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

bool CSelftestPart_Axis::Abort() {
    return true;
}

void CSelftestPart_Axis::ResetHome() {
    m_Home = hsNone;
}

bool CSelftestPart_Axis::next() {
    if ((m_State == spsFinished) || (m_State == spsAborted))
        return false;
    m_State = (TestState)((int)m_State + 1);
    return ((m_State != spsFinished) && (m_State != spsAborted));
}

uint32_t CSelftestPart_Axis::estimate(const selftest_axis_config_t *pconfig) {
    uint32_t total_time = 1000;
    return total_time;
}

CSelftestPart_Axis::HomeState CSelftestPart_Axis::m_Home = hsNone;
