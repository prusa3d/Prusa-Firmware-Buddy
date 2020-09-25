// selftest_axis.h
#pragma once

#include <inttypes.h>
#include "selftest_MINI.h"


class CSelftestPart_Axis : public CSelftestPart {
public:
	enum TestState : uint8_t {
		spsIdle,
		spsStart,
		spsFinish,
		spsFinished,
		spsAborted,
	};
public:
	CSelftestPart_Axis(uint8_t axis);
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
	uint8_t m_Axis;
	TestState m_State;
};
