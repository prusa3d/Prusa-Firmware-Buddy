// selftest_fan.cpp

#include "selftest_fan.h"
#include "fanctl.h"

#define FANTEST_STOP_DELAY    2000
#define FANTEST_WAIT_DELAY    2500
#define FANTEST_MEASURE_DELAY 2500


CSelftestPart_Fan::CSelftestPart_Fan(const char* partname, CFanCtl* pfanctl, int pwm_start, int pwm_step, int pwm_steps) :
	m_pFanCtl(pfanctl),
	m_State(spsStart),
	m_Name(partname),
	m_PWM_Start(pwm_start),
	m_PWM_Step(pwm_step),
	m_PWM_Steps(pwm_steps) {

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
	    Selftest.log_printf("%s Start\n", m_Name);
		m_Time = HAL_GetTick();
		if ((m_pFanCtl->getPWM() == 0) && (m_pFanCtl->getActualRPM() == 0))
			m_Time += FANTEST_STOP_DELAY;
		m_pFanCtl->setPWM(0);
		break;
	case spsWait_stopped:
		if ((HAL_GetTick() - m_Time) <= FANTEST_STOP_DELAY)
			return true;
		m_pFanCtl->setPWM(m_PWM_Start);
		m_Time = HAL_GetTick();
		break;
	case spsWait_rpm:
		if ((HAL_GetTick() - m_Time) <= FANTEST_WAIT_DELAY)
			return true;
		m_Time = HAL_GetTick();
	    m_SampleCount = 0;
	    m_SampleSum = 0;
		break;
	case spsMeasure_rpm:
		if ((HAL_GetTick() - m_Time) <= FANTEST_MEASURE_DELAY) {
		    m_SampleCount++;
		    m_SampleSum += m_pFanCtl->getActualRPM();
			return true;
		}
	    Selftest.log_printf("%s at %u%% PWM = %u RPM\n", m_Name, m_pFanCtl->getPWM(), m_SampleSum / m_SampleCount);
		if (--m_PWM_Steps)
		{
			m_pFanCtl->setPWM(m_pFanCtl->getPWM() + m_PWM_Step);
			m_Time = HAL_GetTick();
			m_State = spsWait_rpm;
			return true;
		}
		break;
	case spsFinish:
		m_pFanCtl->setPWM(0);
	    Selftest.log_printf("%s Start\n", m_Name);
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
	if (m_pFanCtl)
		m_pFanCtl->setPWM(0);
	m_State = spsAborted;
	return true;
}

float CSelftestPart_Fan::GetProgress() {
	return 0;
}

TestResult_t CSelftestPart_Fan::GetResult() {
	return sprUnknown;
}

bool CSelftestPart_Fan::next() {
	if ((m_State == spsFinished) || (m_State == spsAborted))
		return false;
	m_State = (TestState)((int)m_State + 1);
	return ((m_State != spsFinished) && (m_State != spsAborted));
}
