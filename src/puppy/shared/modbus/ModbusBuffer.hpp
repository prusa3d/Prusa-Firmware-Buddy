#pragma once

#include "PuppyConfig.hpp"
#include <cstdint>

namespace modbus {

class ModbusBuffer {
public:
    ModbusBuffer();
    virtual ~ModbusBuffer();

    uint32_t GetActualSize() {
        return m_ActualByteCount;
    }

    uint32_t GetFreeSize() {
        return RS485_BUFFER_SIZE - m_ActualByteCount;
    }

    uint32_t GetProcessedSize() {
        return m_ActualDataIndex;
    }

    uint32_t GetUnprocessedSize() {
        return m_ActualByteCount - m_ActualDataIndex;
    }

    uint8_t &operator[](int index) {
        return m_pBuffer[index];
    }

    void Reset();

    void CopyData(const uint8_t *pData, uint32_t dataSize);

    uint8_t ReadByte();
    uint16_t ReadWord();

    void AddByte(uint8_t value);
    void AddWord(uint16_t value);

private:
    uint8_t m_pBuffer[RS485_BUFFER_SIZE];

    uint32_t m_ActualByteCount;
    uint32_t m_ActualDataIndex;
};

} // namespace modbus
