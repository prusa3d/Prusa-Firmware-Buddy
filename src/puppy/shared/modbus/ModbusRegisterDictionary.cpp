#include "ModbusRegisterDictionary.hpp"
#include <assert.h>
#include <string.h>
#include <span>

namespace modbus::RegisterDictionary {

struct BlockInfo {
    uint16_t *pData;
    uint32_t regAddress;
    uint32_t regCount;
    BlockInfo *pNextBlock;
};

static BlockInfo *s_pFirstDiscreteInputBlock = nullptr;
static BlockInfo *s_pFirstCoilBlock = nullptr;
static BlockInfo *s_pFirstInputRegisterBlock = nullptr;
static BlockInfo *s_pFirstHoldingRegisterBlock = nullptr;

BlockInfo **GetBlockInfoPtr(BlockType type) {
    switch (type) {
    case BlockType::DiscreteInput:
        return &s_pFirstDiscreteInputBlock;
    case BlockType::Coil:
        return &s_pFirstCoilBlock;
    case BlockType::InputRegister:
        return &s_pFirstInputRegisterBlock;
    case BlockType::HoldingRegister:
        return &s_pFirstHoldingRegisterBlock;
    default:
        return nullptr; // unknown block type
    }
}

void AddBlock(BlockType type, uint16_t *pData, uint32_t regAddress, uint32_t regCount) {
    BlockInfo **ppFirstBI = GetBlockInfoPtr(type);

    if (ppFirstBI != nullptr) {
        BlockInfo *pBI = new BlockInfo();
        assert(pBI != nullptr);

        pBI->pData = pData;
        pBI->regAddress = regAddress;
        pBI->regCount = regCount;

        pBI->pNextBlock = *ppFirstBI;
        *ppFirstBI = pBI;
    }
}

BlockInfo *FindBlockInfo(BlockType type, uint32_t regAddress) {
    BlockInfo **ppFirstBI = GetBlockInfoPtr(type);

    if (ppFirstBI != nullptr) {
        BlockInfo *pBI = *ppFirstBI;

        while (pBI != nullptr) {
            if (regAddress >= pBI->regAddress && regAddress < (pBI->regAddress + pBI->regCount)) {
                break;
            }
            pBI = pBI->pNextBlock;
        }

        return pBI;
    } else {
        return nullptr;
    }
}

bool GetDiscreteInputValue(uint16_t address, bool *pValue) {
    BlockInfo *pBI = FindBlockInfo(BlockType::DiscreteInput, address);

    if (pBI != nullptr) {
        *pValue = pBI->pData[address - pBI->regAddress] != 0 ? true : false;
        return true;
    } else {
        return false;
    }
}

bool SetDiscreteInputValue(uint16_t address, bool value) {
    BlockInfo *pBI = FindBlockInfo(BlockType::DiscreteInput, address);

    if (pBI != nullptr) {
        pBI->pData[address - pBI->regAddress] = value ? 1 : 0;
        return true;
    } else {
        return false;
    }
}

bool GetCoilValue(uint16_t address, bool *pValue) {
    BlockInfo *pBI = FindBlockInfo(BlockType::Coil, address);

    if (pBI != nullptr) {
        *pValue = pBI->pData[address - pBI->regAddress] != 0 ? true : false;
        return true;
    } else {
        return false;
    }
}

bool SetCoilValue(uint16_t address, bool value) {
    BlockInfo *pBI = FindBlockInfo(BlockType::Coil, address);

    if (pBI != nullptr) {
        pBI->pData[address - pBI->regAddress] = value ? 1 : 0;
        return true;
    } else {
        return false;
    }
}

bool GetHoldingRegisterValue(uint16_t address, uint16_t *pValue) {
    BlockInfo *pBI = FindBlockInfo(BlockType::HoldingRegister, address);

    if (pBI != nullptr) {
        *pValue = pBI->pData[address - pBI->regAddress];
        return true;
    } else {
        return false;
    }
}

bool SetHoldingRegisterValue(uint16_t address, uint16_t value) {
    BlockInfo *pBI = FindBlockInfo(BlockType::HoldingRegister, address);

    if (pBI != nullptr) {
        pBI->pData[address - pBI->regAddress] = value;
        return true;
    } else {
        return false;
    }
}

bool GetInputRegisterValue(uint16_t address, uint16_t *pValue) {
    BlockInfo *pBI = FindBlockInfo(BlockType::InputRegister, address);

    if (pBI != nullptr) {
        *pValue = pBI->pData[address - pBI->regAddress];
        return true;
    } else {
        return false;
    }
}

bool SetInputRegisterValue(uint16_t address, uint16_t value) {
    BlockInfo *pBI = FindBlockInfo(BlockType::InputRegister, address);

    if (pBI != nullptr) {
        pBI->pData[address - pBI->regAddress] = value;
        return true;
    } else {
        return false;
    }
}

bool PutStringIntoInputRegisters(uint16_t from_address, uint16_t to_address, const char *string) {
    BlockInfo *pBI = FindBlockInfo(BlockType::InputRegister, from_address);
    if (!pBI || from_address > to_address) {
        return false;
    }

    const uint16_t num_registers = to_address + 1 - from_address;
    const size_t from_offset = from_address - pBI->regAddress;
    std::span reg_span(&pBI->pData[from_offset], num_registers);
    auto bytes_span = std::as_writable_bytes(reg_span);
    size_t len = strnlen(string, bytes_span.size());

    if (from_address >= pBI->regAddress && (num_registers + from_offset) <= pBI->regCount) {
        memcpy(bytes_span.data(), string, len);
        return true;
    } else {
        return false;
    }
}

} // namespace modbus::RegisterDictionary
