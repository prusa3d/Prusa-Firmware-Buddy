#pragma once

#include "ModbusRegisterDictionary.hpp"
#include "ModbusBuffer.hpp"
#include "PuppyConfig.hpp"
#include <cstdint>
#include <array>

namespace modbus::ModbusProtocol {

enum class Function : uint8_t {
    ReadCoils = 1,
    ReadDiscreteInputs = 2,
    ReadHoldingRegisters = 3,
    ReadInputRegisters = 4,
    WriteSingleCoil = 5,
    WriteSingleRegister = 6,

    WriteMultipleCoils = 15,
    WriteMultipleRegisters = 16,

    ReadFileRecord = 20,
    WriteFileRecord = 21,

    ReadFIFO = 24,
};

enum class ExceptionCode : uint8_t {
    NoError = 0,
    IllegalFunction = 1,
    IllegalDataAddress = 2,
    IllegalDataValue = 3,
    ServerDeviceFailure = 4,
};

typedef void (*OnReadRegisterCallback)(uint16_t address);
typedef void (*OnWriteCoilCallback)(uint16_t address, bool value);
typedef void (*OnWriteRegisterCallback)(uint16_t address, uint16_t value);
typedef bool (*OnReadFileRecordCallback)(uint16_t fileNumber, uint16_t recordNumber, uint16_t recordLength, ModbusBuffer *pBuffer);
typedef bool (*OnWriteFileRecordCallback)(uint16_t fileNumber, uint16_t recordNumber, uint16_t recordLength, ModbusBuffer *pBuffer);
typedef bool (*OnReadFIFOCallback)(uint16_t address, uint32_t *pValueCount, std::array<uint16_t, MODBUS_FIFO_MAX_COUNT> &fifo);

void Init(uint8_t modbusAddress);

void SetOnReadHoldingRegisterCallback(OnReadRegisterCallback pCallback);
void SetOnReadInputRegisterCallback(OnReadRegisterCallback pCallback);
void SetOnWriteCoilCallback(OnWriteCoilCallback pCallback);
void SetOnWriteRegisterCallback(OnWriteRegisterCallback pCallback);
void SetOnReadFileRecordCallback(OnReadFileRecordCallback pCallback);
void SetOnWriteFileRecordCallback(OnWriteFileRecordCallback pCallback);
void SetOnReadFIFOCallback(OnReadFIFOCallback pCallback);

bool ProcessFrame(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);

} // namespace modbus::ModbusProtocol
