#pragma once

#include "MeasurementLogic.hpp"
#include "StateLogic.hpp"
#include "PuppyConfig.hpp"
#include <cstdint>

namespace modularbed::AutoconfLogic {

void CheckHeatbedlets();
void MeasureAndCheckAllHBCurrents();
void MeasureAndCheckSingleHBCurrent(uint32_t hbIndex);
void CalibrateThermistors();
void StartTestHBHeating();
void StopTestHBHeating();

void IterateTesting();

} // namespace modularbed::AutoconfLogic
