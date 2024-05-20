#pragma once

#include "i2c.hpp"

// system config address registr
//  constexpr const uint16_t REG_GPO = 0x0000;
//  constexpr const uint16_t REG_IT_TIME = 0x0001;
//  constexpr const uint16_t REG_EH_MODE = 0x0002;
// constexpr const uint16_t REG_RF_MNGT = 0x0003;
constexpr const uint16_t REG_RFA1SS = 0x0004;
constexpr const uint16_t REG_ENDA1 = 0x0005;
constexpr const uint16_t REG_RFA2SS = 0x0006;
constexpr const uint16_t REG_ENDA2 = 0x0007;
constexpr const uint16_t REG_RFA3SS = 0x0008;
constexpr const uint16_t REG_ENDA3 = 0x0009;
// constexpr const uint16_t REG_RFA4SS = 0x000A;
// constexpr const uint16_t REG_I2CSS = 0x000B;
constexpr const uint16_t REG_LOCK_CCFILE = 0x000C;
// constexpr const uint16_t REG_MB_MODE = 0x000D;
// constexpr const uint16_t REG_MB_WDG = 0x000E;
// constexpr const uint16_t REG_LOCK_CFG = 0x000F;
constexpr const uint16_t MEM_RF_MNGT_Dyn = 0x2003;
constexpr const uint16_t MEM_IT_STS_Dyn = 0x2005;

// EEPROM I2C addresses
enum class EepromCommand : bool {
    memory,
    registers
};

enum class EepromCommandWrite : uint16_t {
    addr_memory = 0xA6,
    addr_registers = 0xAE
};

enum class EepromCommandRead : uint16_t {
    addr_memory = 0xA7,
    addr_registers = 0xAF
};

static constexpr EepromCommandWrite eeprom_get_write_address(EepromCommand cmd) {
    if (cmd == EepromCommand::memory) {
        return EepromCommandWrite::addr_memory;
    }
    return EepromCommandWrite::addr_registers;
}

static constexpr EepromCommandRead eeprom_get_read_address(EepromCommand cmd) {
    if (cmd == EepromCommand::memory) {
        return EepromCommandRead::addr_memory;
    }
    return EepromCommandRead::addr_registers;
}

void rise_error_if_needed(i2c::Result result);
void try_fix_if_needed(const i2c::Result &result);

[[nodiscard]] i2c::Result eeprom_transmit(EepromCommandWrite cmd, uint8_t *pData, uint16_t size);
[[nodiscard]] i2c::Result user_read_bytes(EepromCommand cmd, uint16_t address, void *pdata, uint16_t size);
[[nodiscard]] i2c::Result user_write_bytes(EepromCommand cmd, uint16_t address, const void *pdata, uint16_t size);
[[nodiscard]] i2c::Result user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size);
