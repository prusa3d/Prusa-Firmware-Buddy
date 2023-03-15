#pragma once

#include "ModbusRegisters.hpp"
#include "hal/HAL_ADC.hpp"
#include "HeatbedletInfo.hpp"
#include "PuppyConfig.hpp"
#include <cstdint>

namespace modularbed::PWMLogic {

void Init();

void EnablePWM();
void DisablePWM();

float GetExpectedCurrent_A();
float GetExpectedCurrent_B();

void ApplyPWMValues();
void ApplyPWMValuesWithoutLimiters();

hal::ADCDriver::ADCChannel GetHBCurrentMeasurementChannel(uint32_t heatbedletIndex);

} //namespace
