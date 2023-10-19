/// @file eeprom.h
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace hal {

/// EEPROM interface
namespace eeprom {

class EEPROM {
public:
#ifdef UNITTEST
    using addr_t = size_t;
#else
    using addr_t = uint16_t;
#endif

    static void WriteByte(addr_t addr, uint8_t value);
    static void UpdateByte(addr_t addr, uint8_t value);
    static uint8_t ReadByte(addr_t addr);

    /// Convenience function to read a 1-byte value from EEPROM and check for unitialized EEPROM cells.
    /// @returns 1-byte value read from the EEPROM.
    /// In case the EEPROM has a default value at @p addr, this function returns @p defaultValue
    static uint8_t ReadByte(addr_t addr, uint8_t defaultValue);

    static void WriteWord(addr_t addr, uint16_t value);
    static void UpdateWord(addr_t addr, uint16_t value);
    static uint16_t ReadWord(addr_t addr);

    /// @returns physical end address of EEPROM memory end
    /// TODO this is sad - the constexpr must be inline... find a way around in the future
    constexpr static addr_t End() { return 2048; }
};

extern EEPROM eeprom;

} // namespace EEPROM
} // namespace hal
