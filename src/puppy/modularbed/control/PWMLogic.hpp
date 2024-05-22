#pragma once

#include "ModbusRegisters.hpp"
#include "hal/HAL_ADC.hpp"
#include "HeatbedletInfo.hpp"
#include "PuppyConfig.hpp"
#include <cstdint>
#include "utility_extensions.hpp"

namespace modularbed::PWMLogic {

void Init();

void EnablePWM();
void DisablePWM();

float GetExpectedCurrent(const uint8_t idx);

void ApplyPWMValues();
void ApplyPWMValuesWithoutLimiters();

hal::ADCDriver::ADCChannel GetHBCurrentMeasurementChannel(uint32_t heatbedletIndex);

} // namespace modularbed::PWMLogic
