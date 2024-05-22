#pragma once

#include "ModbusRegisters.hpp"

namespace modularbed::ControlLogic {

void Init();

void EnableControllers();
void DisableControllers();

void IterateContollers();

} // namespace modularbed::ControlLogic
