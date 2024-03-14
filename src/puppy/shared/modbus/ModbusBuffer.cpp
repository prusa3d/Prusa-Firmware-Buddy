#include "ModbusBuffer.hpp"
#include <cstring>

namespace modbus {

ModbusBuffer::ModbusBuffer() {
    Reset();
}

ModbusBuffer::~ModbusBuffer() {
}

void ModbusBuffer::Reset() {
    m_ActualByteCount = 0;
    m_ActualDataIndex = 0;
}

void ModbusBuffer::CopyData(const uint8_t *pData, uint32_t dataSize) {
    if (dataSize > RS485_BUFFER_SIZE) {
        dataSize = RS485_BUFFER_SIZE;
    }

    memcpy(m_pBuffer, pData, dataSize);
    m_ActualByteCount = dataSize;
    m_ActualDataIndex = 0;
}

uint8_t ModbusBuffer::ReadByte() {
    if (m_ActualDataIndex >= m_ActualByteCount) {
        return 0;
    }

    uint8_t value = m_pBuffer[m_ActualDataIndex];
    m_ActualDataIndex++;

    return value;
}

uint16_t ModbusBuffer::ReadWord() {
    if ((m_ActualDataIndex + 2) > m_ActualByteCount) {
        return 0;
    }

    uint16_t value = (m_pBuffer[m_ActualDataIndex] << 8) + m_pBuffer[m_ActualDataIndex + 1];
    m_ActualDataIndex += 2;

    return value;
}

void ModbusBuffer::AddByte(uint8_t value) {
    if (m_ActualByteCount < RS485_BUFFER_SIZE) {
        m_pBuffer[m_ActualByteCount] = value;
        m_ActualByteCount++;
    }
}

void ModbusBuffer::AddWord(uint16_t value) {
    if ((m_ActualByteCount + 1) < RS485_BUFFER_SIZE) {
        m_pBuffer[m_ActualByteCount] = value >> 8;
        m_pBuffer[m_ActualByteCount + 1] = (uint8_t)value;
        m_ActualByteCount += 2;
    }
}

} // namespace modbus
