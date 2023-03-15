#pragma once

#include "hal/HAL_ADC.hpp"
#include <cstdint>

namespace modularbed {

float CalcThermistorResistance(hal::ADCDriver::ADCChannel channel, uint16_t adcValue);

float CalcThermistorTemperature(float resistance);

float CalcElectricCurrent(uint16_t adcValue);

float CalcHBReferenceResistance(float current, float temperature);

float CalcHBResistanceAtTemperature(float referenceResistance, float temperature);

} //namespace
