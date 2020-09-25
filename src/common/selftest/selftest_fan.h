// selftest_fan.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"
#include "fanctl.h"


class CSelftestPart_Fan : public CSelftestPart {
public:
	enum TestState : uint8_t {
		spsIdle,
		spsStart,
		spsWait_stopped,
		spsWait_rpm,
		spsMeasure_rpm,
		spsFinish,
		spsFinished,
		spsAborted,
	};
public:
	CSelftestPart_Fan(const char* partname, CFanCtl* pfanctl, int pwm_start, int pwm_step, int pwm_steps);
public:
	bool IsInProgress() const;
public:
	bool Start();
	bool Loop();
	bool Abort();
	float GetProgress();
	TestResult_t GetResult();
protected:
	bool next();
protected:
	CFanCtl* m_pFanCtl;
	TestState m_State;
	uint32_t m_Time;
	const char* m_Name;
	uint8_t m_PWM_Start;
	uint8_t m_PWM_Step;
	uint8_t m_PWM_Steps;
    uint16_t m_SampleCount;
    uint32_t m_SampleSum;
};
