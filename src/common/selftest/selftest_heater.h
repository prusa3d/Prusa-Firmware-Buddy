// selftest_heater.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"


class CSelftestPart_Heater : public CSelftestPart {
public:
	enum TestState : uint8_t {
		spsIdle,
		spsStart,
		spsFinish,
		spsFinished,
		spsAborted,
	};
public:
	CSelftestPart_Heater(uint8_t heater);
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
	uint8_t m_Heater;
	TestState m_State;
};
