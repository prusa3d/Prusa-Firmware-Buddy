/// @file eeprom.h
#pragma once
#include <stdint.h>

namespace hal {

/// EEPROM interface
namespace eeprom {

class EEPROM {
public:
    using addr_t = uint16_t;

    static void WriteByte(addr_t addr, uint8_t value);
    static void UpdateByte(addr_t addr, uint8_t value);
    static uint8_t ReadByte(addr_t addr);

    static void WriteWord(addr_t addr, uint16_t value);
    static void UpdateWord(addr_t addr, uint16_t value);
    static uint16_t ReadWord(addr_t addr);

    /// @returns physical end address of EEPROM memory end
    /// @@TODO this is sad - the constexpr must be inline... find a way around in the future
    constexpr static addr_t End() { return 2048; }
};

extern EEPROM eeprom;

} // namespace EEPROM
} // namespace hal
