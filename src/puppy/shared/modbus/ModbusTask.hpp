#pragma once

#include "hal/HAL_RS485.hpp"
#include "ModbusProtocol.hpp"
#include "PuppyConfig.hpp"

namespace modbus::ModbusTask {

bool Init();

void EnableModbus();
void DisableModbus();

} // namespace modbus::ModbusTask
