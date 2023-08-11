#pragma once

#include "hal/HAL_ADC.hpp"
#include "hal/HAL_PWM.hpp"
#include "MovingAverageFilter.hpp"
#include "cmsis_os.h"
#include "PuppyConfig.hpp"

namespace modularbed::MeasurementLogic {

void Init();

void CalibrateCurrentChannels();

void StartADCMeasurements();
bool IterateADCMeasurements(hal::ADCDriver::ADCChannel *pChannel, float *value);

float MeasureSingleChannel(hal::ADCDriver::ADCChannel channel);
float PreciselyMeasureChannel(hal::ADCDriver::ADCChannel channel);

float GetLastMeasuredAndCalculatedValue(hal::ADCDriver::ADCChannel channel);

} // namespace modularbed::MeasurementLogic
