// selftest_axis.cpp

#include "selftest_axis.h"


CSelftestPart_Axis::CSelftestPart_Axis(uint8_t axis) :
	m_Axis(axis) {

}

bool CSelftestPart_Axis::Start() {
	return true;
}

bool CSelftestPart_Axis::IsInProgress() const {
	return false;
}

bool CSelftestPart_Axis::Loop() {
	return true;
}

bool CSelftestPart_Axis::Abort() {
	return true;
}

float CSelftestPart_Axis::GetProgress() {
	return 0;
}

TestResult_t CSelftestPart_Axis::GetResult() {
	return sprUnknown;
}

bool CSelftestPart_Axis::next() {
	return true;
}
