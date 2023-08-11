#pragma once

#include "MeasurementLogic.hpp"
#include "AutoconfLogic.hpp"
#include "StateLogic.hpp"
#include "hal/HAL_ADC.hpp"
#include "hal/HAL_PWM.hpp"
#include "modbus/ModbusProtocol.hpp"
#include "ModbusRegisters.hpp"
#include "modbus/ModbusBuffer.hpp"
#include "cmsis_os.h"
#include "PuppyConfig.hpp"

namespace modularbed::ControlTask {

bool Init();

} // namespace modularbed::ControlTask
