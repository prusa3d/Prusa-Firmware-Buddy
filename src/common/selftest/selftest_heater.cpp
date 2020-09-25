// selftest_heater.cpp

#include "selftest_heater.h"


CSelftestPart_Heater::CSelftestPart_Heater(uint8_t heater) :
	m_Heater(heater) {

}

bool CSelftestPart_Heater::IsInProgress() const {
	return false;
}

bool CSelftestPart_Heater::Start() {
	return true;
}

bool CSelftestPart_Heater::Loop() {
	return true;
}

bool CSelftestPart_Heater::Abort() {
	return true;
}

float CSelftestPart_Heater::GetProgress() {
	return 0;
}

TestResult_t CSelftestPart_Heater::GetResult() {
	return sprUnknown;
}

bool CSelftestPart_Heater::next() {
	return true;
}
