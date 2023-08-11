#pragma once

#include <cstdint>

namespace modbus::RegisterDictionary {

enum class BlockType {
    DiscreteInput,
    Coil,
    InputRegister,
    HoldingRegister
};

void AddBlock(BlockType type, uint16_t *pData, uint32_t regAddress, uint32_t regCount);

bool GetDiscreteInputValue(uint16_t address, bool *pValue);
bool SetDiscreteInputValue(uint16_t address, bool value);

bool GetCoilValue(uint16_t address, bool *pValue);
bool SetCoilValue(uint16_t address, bool value);

bool GetHoldingRegisterValue(uint16_t address, uint16_t *pValue);
bool SetHoldingRegisterValue(uint16_t address, uint16_t value);

bool GetInputRegisterValue(uint16_t address, uint16_t *pValue);
bool SetInputRegisterValue(uint16_t address, uint16_t value);
bool PutStringIntoInputRegisters(uint16_t from_address, uint16_t to_address, const char *string);

} // namespace modbus::RegisterDictionary
