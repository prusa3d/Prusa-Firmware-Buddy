#include "ModbusProtocol.hpp"

#define MSG_OFFSET_ADDRESS  0
#define MSG_OFFSET_FUNCTION 1
#define MIN_MESSAGE_SIZE    4
#define CRC_FIELD_SIZE      2
#define ERROR_CODE_FLAG     0x80

#define MAX_BIT_VALUES_QUANTITY  2000 // limited by Modbus specification
#define MAX_WORD_VALUES_QUANTITY 125 // limited by Modbus specification
#define MAX_REG_VALUES_QUANTITY  123 // limited by Modbus specification
#define WRITE_VALUE_ON           0xFF00
#define WRITE_VALUE_OFF          0x0000

#define MAX_READ_FILE_QUANTITY    35 // limited by Modbus specification
#define MAX_WRITE_FILE_QUANTITY   27 // limited by Modbus specification
#define READ_FILE_REQUEST_SIZE    7
#define READ_FILE_REFERENCE_TYPE  6
#define MAX_FILE_FRAME_BYTE_COUNT 245 // limited by Modbus specification
#define MIN_WRITE_FILE_BYTE_COUNT 9
#define MAX_WRITE_FILE_BYTE_COUNT 251
#define WRITE_FILE_HEADER_SIZE    7

namespace modbus::ModbusProtocol {

uint8_t m_ModbusAddress = 0;

static OnReadRegisterCallback s_pOnReadHoldingRegister = nullptr;
static OnReadRegisterCallback s_pOnReadInputRegister = nullptr;
static OnWriteCoilCallback s_pOnWriteCoil = nullptr;
static OnWriteRegisterCallback s_pOnWriteRegister = nullptr;
static OnReadFileRecordCallback s_pOnReadFileRecord = nullptr;
static OnWriteFileRecordCallback s_pOnWriteFileRecord = nullptr;
static OnReadFIFOCallback s_pOnReadFIFO = nullptr;

ExceptionCode ProcessFunction_ReadCoils(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_ReadDiscreteInputs(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_ReadHoldingRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_ReadInputRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_WriteSingleCoil(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_WriteSingleRegister(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_WriteMultipleCoils(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_WriteMultipleRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_ReadFileRecord(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_WriteFileRecord(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);
ExceptionCode ProcessFunction_ReadFIFO(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer);

void GenerateErrorResponse(Function function, ExceptionCode exception, ModbusBuffer *pBuffer);

bool ValidateChecksum(ModbusBuffer *pBuffer);
void AddChecksum(ModbusBuffer *pBuffer);

uint16_t CalculateChecksum(ModbusBuffer *pBuffer, uint32_t dataSize);

void Init(uint8_t modbusAddress) {
    m_ModbusAddress = modbusAddress;
}

void SetOnReadHoldingRegisterCallback(OnReadRegisterCallback pCallback) {
    s_pOnReadHoldingRegister = pCallback;
}

void SetOnReadInputRegisterCallback(OnReadRegisterCallback pCallback) {
    s_pOnReadInputRegister = pCallback;
}

void SetOnWriteCoilCallback(OnWriteCoilCallback pCallback) {
    s_pOnWriteCoil = pCallback;
}

void SetOnWriteRegisterCallback(OnWriteRegisterCallback pCallback) {
    s_pOnWriteRegister = pCallback;
}

void SetOnReadFileRecordCallback(OnReadFileRecordCallback pCallback) {
    s_pOnReadFileRecord = pCallback;
}

void SetOnWriteFileRecordCallback(OnWriteFileRecordCallback pCallback) {
    s_pOnWriteFileRecord = pCallback;
}

void SetOnReadFIFOCallback(OnReadFIFOCallback pCallback) {
    s_pOnReadFIFO = pCallback;
}

bool ProcessFrame(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    if (pRX_Buffer->GetUnprocessedSize() < MIN_MESSAGE_SIZE) {
        return false; // frame is too short
    }

    uint8_t modbusAddress = pRX_Buffer->ReadByte();
    Function function = (Function)pRX_Buffer->ReadByte();

    if (modbusAddress != m_ModbusAddress) {
        return false; // not our message
    }

    if (ValidateChecksum(pRX_Buffer) == false) {
        return false; // CRC error - no response
    }

    // prepare response header
    pTX_Buffer->Reset();
    pTX_Buffer->AddByte(m_ModbusAddress);
    pTX_Buffer->AddByte((uint8_t)function);
    ExceptionCode exception = ExceptionCode::NoError;

    switch (function) {
    case Function::ReadCoils:
        exception = ProcessFunction_ReadCoils(pRX_Buffer, pTX_Buffer);
        break;
    case Function::ReadDiscreteInputs:
        exception = ProcessFunction_ReadDiscreteInputs(pRX_Buffer, pTX_Buffer);
        break;
    case Function::ReadHoldingRegisters:
        exception = ProcessFunction_ReadHoldingRegisters(pRX_Buffer, pTX_Buffer);
        break;
    case Function::ReadInputRegisters:
        exception = ProcessFunction_ReadInputRegisters(pRX_Buffer, pTX_Buffer);
        break;
    case Function::WriteSingleCoil:
        exception = ProcessFunction_WriteSingleCoil(pRX_Buffer, pTX_Buffer);
        break;
    case Function::WriteSingleRegister:
        exception = ProcessFunction_WriteSingleRegister(pRX_Buffer, pTX_Buffer);
        break;
    case Function::WriteMultipleCoils:
        exception = ProcessFunction_WriteMultipleCoils(pRX_Buffer, pTX_Buffer);
        break;
    case Function::WriteMultipleRegisters:
        exception = ProcessFunction_WriteMultipleRegisters(pRX_Buffer, pTX_Buffer);
        break;
    case Function::ReadFileRecord:
        exception = ProcessFunction_ReadFileRecord(pRX_Buffer, pTX_Buffer);
        break;
    case Function::WriteFileRecord:
        exception = ProcessFunction_WriteFileRecord(pRX_Buffer, pTX_Buffer);
        break;
    case Function::ReadFIFO:
        exception = ProcessFunction_ReadFIFO(pRX_Buffer, pTX_Buffer);
        break;
    default:
        exception = ExceptionCode::IllegalFunction;
        break;
    }

    if (exception == ExceptionCode::NoError) {
        AddChecksum(pTX_Buffer);
    } else {
        GenerateErrorResponse(function, exception, pTX_Buffer);
    }

    return true;
}

ExceptionCode ProcessFunction_ReadCoils(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t startingAddress = pRX_Buffer->ReadWord();
    uint32_t quantity = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    if (quantity < 1 || quantity > MAX_BIT_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t byteCount = quantity / 8;
    if ((quantity % 8) != 0) {
        byteCount++;
    }

    pTX_Buffer->AddByte(byteCount);

    uint32_t dataByte = 0;
    uint32_t bitsInByte = 0;

    for (uint32_t i = 0; i < quantity; i++) {
        bool value;
        if (RegisterDictionary::GetCoilValue(startingAddress + i, &value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        if (value) {
            dataByte |= 1 << bitsInByte;
        }
        bitsInByte++;

        if (bitsInByte == 8) {
            pTX_Buffer->AddByte(dataByte);
            dataByte = 0;
            bitsInByte = 0;
        }
    }

    if (bitsInByte > 0) {
        pTX_Buffer->AddByte(dataByte);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_ReadDiscreteInputs(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t startingAddress = pRX_Buffer->ReadWord();
    uint32_t quantity = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    if (quantity < 1 || quantity > MAX_BIT_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t byteCount = quantity / 8;
    if ((quantity % 8) != 0) {
        byteCount++;
    }

    pTX_Buffer->AddByte(byteCount);

    uint32_t dataByte = 0;
    uint32_t bitsInByte = 0;

    for (uint32_t i = 0; i < quantity; i++) {
        bool value;
        if (RegisterDictionary::GetDiscreteInputValue(startingAddress + i, &value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        if (value) {
            dataByte |= 1 << bitsInByte;
        }
        bitsInByte++;

        if (bitsInByte == 8) {
            pTX_Buffer->AddByte(dataByte);
            dataByte = 0;
            bitsInByte = 0;
        }
    }

    if (bitsInByte > 0) {
        pTX_Buffer->AddByte(dataByte);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_ReadHoldingRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint16_t startingAddress = pRX_Buffer->ReadWord();
    uint16_t quantity = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    if (quantity < 1 || quantity > MAX_WORD_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t byteCount = quantity * 2;
    pTX_Buffer->AddByte(byteCount);

    if (s_pOnReadHoldingRegister != nullptr) {
        for (uint32_t i = 0; i < quantity; i++) {
            s_pOnReadHoldingRegister(startingAddress + i);
        }
    }

    for (uint32_t i = 0; i < quantity; i++) {
        uint16_t value;
        if (RegisterDictionary::GetHoldingRegisterValue(startingAddress + i, &value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        pTX_Buffer->AddWord(value);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_ReadInputRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint16_t startingAddress = pRX_Buffer->ReadWord();
    uint16_t quantity = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    if (quantity < 1 || quantity > MAX_WORD_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t byteCount = quantity * 2;
    pTX_Buffer->AddByte(byteCount);

    if (s_pOnReadInputRegister != nullptr) {
        for (uint32_t i = 0; i < quantity; i++) {
            s_pOnReadInputRegister(startingAddress + i);
        }
    }

    for (uint32_t i = 0; i < quantity; i++) {
        uint16_t value;
        if (RegisterDictionary::GetInputRegisterValue(startingAddress + i, &value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        pTX_Buffer->AddWord(value);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_WriteSingleCoil(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint16_t address = pRX_Buffer->ReadWord();
    uint16_t value = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    bool boolValue;
    if (value == WRITE_VALUE_ON) {
        boolValue = true;
    } else if (value == WRITE_VALUE_OFF) {
        boolValue = false;
    } else {
        return ExceptionCode::IllegalDataValue;
    }

    pTX_Buffer->AddWord(address);
    pTX_Buffer->AddWord(value);

    if (RegisterDictionary::SetCoilValue(address, boolValue) == false) {
        return ExceptionCode::IllegalDataAddress;
    }

    if (s_pOnWriteCoil != nullptr) {
        s_pOnWriteCoil(address, boolValue);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_WriteSingleRegister(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint16_t address = pRX_Buffer->ReadWord();
    uint16_t value = pRX_Buffer->ReadWord();

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    pTX_Buffer->AddWord(address);
    pTX_Buffer->AddWord(value);

    if (RegisterDictionary::SetHoldingRegisterValue(address, value) == false) {
        return ExceptionCode::IllegalDataAddress;
    }

    if (s_pOnWriteRegister != nullptr) {
        s_pOnWriteRegister(address, value);
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_WriteMultipleCoils(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t startingAddress = pRX_Buffer->ReadWord();
    uint32_t quantity = pRX_Buffer->ReadWord();
    uint32_t byteCount = pRX_Buffer->ReadByte();

    if (quantity < 1 || quantity > MAX_BIT_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t expectedByteCount = quantity / 8;
    if ((quantity % 8) != 0) {
        expectedByteCount++;
    }

    if (byteCount != expectedByteCount) {
        return ExceptionCode::IllegalDataValue;
    }

    pTX_Buffer->AddWord(startingAddress);
    pTX_Buffer->AddWord(quantity);

    uint32_t dataByte = pRX_Buffer->ReadByte();
    uint32_t bitsInByte = 0;

    for (uint32_t i = 0; i < quantity; i++) {
        bool value = ((dataByte >> bitsInByte) & 1) == 1;

        if (RegisterDictionary::SetCoilValue(startingAddress + i, value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        if (s_pOnWriteCoil != nullptr) {
            s_pOnWriteCoil(startingAddress + i, value);
        }

        bitsInByte++;
        if (bitsInByte == 8) {
            dataByte = pRX_Buffer->ReadByte();
            bitsInByte = 0;
        }
    }

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_WriteMultipleRegisters(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t startingAddress = pRX_Buffer->ReadWord();
    uint32_t quantity = pRX_Buffer->ReadWord();
    uint32_t byteCount = pRX_Buffer->ReadByte();

    if (quantity < 1 || quantity > MAX_REG_VALUES_QUANTITY) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t expectedByteCount = quantity * 2;

    if (byteCount != expectedByteCount) {
        return ExceptionCode::IllegalDataValue;
    }

    pTX_Buffer->AddWord(startingAddress);
    pTX_Buffer->AddWord(quantity);

    for (uint32_t i = 0; i < quantity; i++) {
        uint32_t value = pRX_Buffer->ReadWord();

        if (RegisterDictionary::SetHoldingRegisterValue(startingAddress + i, value) == false) {
            return ExceptionCode::IllegalDataAddress;
        }

        if (s_pOnWriteRegister != nullptr) {
            s_pOnWriteRegister(startingAddress + i, value);
        }
    }

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_ReadFileRecord(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t byteCount = pRX_Buffer->ReadByte();

    uint32_t quantity = byteCount / READ_FILE_REQUEST_SIZE;

    if (quantity == 0 || quantity > MAX_READ_FILE_QUANTITY || (byteCount % READ_FILE_REQUEST_SIZE) != 0) {
        return ExceptionCode::IllegalDataValue;
    }

    int byteCountIndex = pTX_Buffer->GetActualSize();
    pTX_Buffer->AddByte(0); // byte count

    for (uint32_t i = 0; i < quantity; i++) {
        uint8_t referenceType = pRX_Buffer->ReadByte();
        if (referenceType != READ_FILE_REFERENCE_TYPE) {
            return ExceptionCode::IllegalDataAddress;
        }

        uint16_t fileNumber = pRX_Buffer->ReadWord();
        uint16_t recordNumber = pRX_Buffer->ReadWord();
        uint16_t recordLength = pRX_Buffer->ReadWord();

        uint32_t subByteCount = recordLength * 2 + 1;

        if ((((uint32_t)(*pTX_Buffer)[byteCountIndex]) + subByteCount + 1) > MAX_FILE_FRAME_BYTE_COUNT) {
            return ExceptionCode::IllegalDataValue;
        }
        (*pTX_Buffer)[byteCountIndex] += subByteCount + 1;

        pTX_Buffer->AddByte(subByteCount);
        pTX_Buffer->AddByte(READ_FILE_REFERENCE_TYPE);

        if (s_pOnReadFileRecord == nullptr || s_pOnReadFileRecord(fileNumber, recordNumber, recordLength, pTX_Buffer) == false) {
            return ExceptionCode::IllegalDataAddress;
        }
    }

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_WriteFileRecord(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t byteCount = pRX_Buffer->ReadByte();

    if (byteCount < MIN_WRITE_FILE_BYTE_COUNT || byteCount > MAX_WRITE_FILE_BYTE_COUNT) {
        return ExceptionCode::IllegalDataValue;
    }

    uint32_t consumedByteCount = 0;

    while (consumedByteCount < byteCount) {
        uint8_t referenceType = pRX_Buffer->ReadByte();
        if (referenceType != READ_FILE_REFERENCE_TYPE) {
            return ExceptionCode::IllegalDataAddress;
        }

        uint16_t fileNumber = pRX_Buffer->ReadWord();
        uint16_t recordNumber = pRX_Buffer->ReadWord();
        uint16_t recordLength = pRX_Buffer->ReadWord();

        consumedByteCount += recordLength * 2 + WRITE_FILE_HEADER_SIZE;

        if (s_pOnWriteFileRecord == nullptr || s_pOnWriteFileRecord(fileNumber, recordNumber, recordLength, pTX_Buffer) == false) {
            return ExceptionCode::IllegalDataAddress;
        }
    }

    if (pRX_Buffer->GetUnprocessedSize() != CRC_FIELD_SIZE) {
        return ExceptionCode::IllegalFunction;
    }

    pTX_Buffer->CopyData(&((*pRX_Buffer)[0]), pRX_Buffer->GetProcessedSize());

    return ExceptionCode::NoError;
}

ExceptionCode ProcessFunction_ReadFIFO(ModbusBuffer *pRX_Buffer, ModbusBuffer *pTX_Buffer) {
    uint32_t startingAddress = pRX_Buffer->ReadWord();

    // Get FIFO data
    std::array<uint16_t, MODBUS_FIFO_MAX_COUNT> read_buffer;
    uint32_t num_registers = 0;

    if (s_pOnReadFIFO == nullptr || s_pOnReadFIFO((uint16_t)startingAddress, &num_registers, read_buffer) == false) {
        return ExceptionCode::IllegalDataAddress;
    }

    // Fill in reponse data
    pTX_Buffer->AddWord(2 + 2 * num_registers); // Number of bytes in response
    pTX_Buffer->AddWord(num_registers); // Number of register in response
    // Reponse registers
    for (uint16_t i = 0; i < num_registers; ++i) {
        pTX_Buffer->AddWord(read_buffer[i]);
    }

    return ExceptionCode::NoError;
}

void GenerateErrorResponse(Function function, ExceptionCode exception, ModbusBuffer *pBuffer) {
    pBuffer->Reset();
    pBuffer->AddByte(m_ModbusAddress);
    pBuffer->AddByte((uint8_t)function | ERROR_CODE_FLAG);
    pBuffer->AddByte((uint8_t)exception);
    AddChecksum(pBuffer);
}

bool ValidateChecksum(ModbusBuffer *pBuffer) {
    uint32_t actualSize = pBuffer->GetActualSize();

    if (actualSize < 2) {
        return false;
    }

    uint16_t crc = CalculateChecksum(pBuffer, actualSize - 2);

    if ((*pBuffer)[actualSize - 2] == (crc & 0xFF) && (*pBuffer)[actualSize - 1] == (crc >> 8 & 0xFF)) {
        return true;
    } else {
        return false;
    }
}

void AddChecksum(ModbusBuffer *pBuffer) {
    uint16_t crc = CalculateChecksum(pBuffer, pBuffer->GetActualSize());
    pBuffer->AddByte(crc & 0xFF);
    pBuffer->AddByte(crc >> 8 & 0xFF);
}

// http://www.modbustools.com/modbus_crc16.htm
static const uint16_t s_CRCTable[] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t CalculateChecksum(ModbusBuffer *pBuffer, uint32_t dataSize) {
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < dataSize; i++) {
        uint8_t tmp = (uint8_t)((*pBuffer)[i] ^ crc);
        crc >>= 8;
        crc ^= s_CRCTable[tmp];
    }

    return crc;
}

} // namespace modbus::ModbusProtocol
