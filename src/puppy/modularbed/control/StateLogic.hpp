#pragma once

#include "ModbusRegisters.hpp"
#include "hal/HAL_ADC.hpp"
#include "HeatbedletInfo.hpp"
#include "MovingAverageFilter.hpp"
#include "PuppyConfig.hpp"
#include <cstdint>

namespace modularbed::StateLogic {

void SetHBResistance(uint32_t heatbedletIndex, float resistance);

void ProcessMeasuredValue(hal::ADCDriver::ADCChannel channel, float value);
void ProcessMeasuredHeatbedletTemperature(hal::ADCDriver::ADCChannel channel, float temperature);
void ProcessMeasuredElectricCurrent(hal::ADCDriver::ADCChannel channel, float current);

void TargetTemperatureChanged(uint32_t heatbedletIndex, float temperatureDegreesC);
void SignalsRefreshed(bool panic, bool fault);

void SetHBErrorFlag(uint32_t heatbedletIndex, HeatbedletError error);
void ClearHBErrorBits(uint32_t heatbedletIndex, uint32_t errorMask);
void ClearSystemErrorBits(uint32_t errorMask);

bool IsPowerPanicActive();
bool IsCurrentFaultActive();
bool IsAnyFaultActive();

} // namespace modularbed::StateLogic
