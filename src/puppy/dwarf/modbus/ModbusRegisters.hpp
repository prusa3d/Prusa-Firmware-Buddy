#pragma once

#include "ModbusRegisterDictionary.hpp"
#include <cstdint>
#include <array>
#include <functional>
#include <dwarf_registers.hpp>

namespace dwarf::ModbusRegisters {

using namespace modbus::RegisterDictionary;
using namespace dwarf_shared::registers;

void Init();

void SetBitValue(SystemDiscreteInput reg, bool value);
void SetBitValue(SystemCoil reg, bool value);
void SetRegValue(SystemInputRegister reg, uint16_t value);
void SetRegValue(SystemHoldingRegister reg, uint16_t value);
uint16_t GetRegValue(SystemInputRegister reg);
uint16_t GetRegValue(SystemHoldingRegister reg);

} // namespace dwarf::ModbusRegisters
