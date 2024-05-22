#pragma once

#include "ModbusRegisterDictionary.hpp"
#include <cstdint>
#include <modular_bed_registers.hpp>

namespace modularbed::ModbusRegisters {

using namespace modbus::RegisterDictionary;
using namespace modular_bed_shared::registers;

void Init();

void SetBitValue(SystemDiscreteInput reg, bool value);
void SetBitValue(HBDiscreteInput reg, uint16_t heatbedletIndex, bool value);
void SetBitValue(SystemCoil reg, bool value);
void SetRegValue(SystemInputRegister reg, uint16_t value);
void SetRegValue(HBInputRegister reg, uint16_t heatbedletIndex, uint16_t value);
void SetRegValue(SystemHoldingRegister reg, uint16_t value);
void SetRegValue(HBHoldingRegister reg, uint16_t heatbedletIndex, uint16_t value);
uint16_t GetRegValue(SystemInputRegister reg);
uint16_t GetRegValue(HBInputRegister reg, uint16_t heatbedletIndex);
uint16_t GetRegValue(SystemHoldingRegister reg);
uint16_t GetRegValue(HBHoldingRegister reg, uint16_t heatbedletIndex);
bool GetBitValue(SystemCoil reg);

} // namespace modularbed::ModbusRegisters
