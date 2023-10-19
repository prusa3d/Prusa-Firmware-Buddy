/// @file eeprom.cpp
#include "../eeprom.h"
#include <avr/eeprom.h>

namespace hal {
namespace eeprom {

EEPROM eeprom;

uint8_t EEPROM::ReadByte(EEPROM::addr_t addr) {
    return eeprom_read_byte((const uint8_t *)addr);
}

uint8_t EEPROM::ReadByte(EEPROM::addr_t addr, uint8_t defaultValue) {
    uint8_t v = ReadByte(addr);
    return v == 0xff ? defaultValue : v;
}

void EEPROM::UpdateByte(EEPROM::addr_t addr, uint8_t value) {
    eeprom_update_byte((uint8_t *)addr, value);
}

uint16_t EEPROM::ReadWord(EEPROM::addr_t addr) {
    return eeprom_read_word((const uint16_t *)addr);
}

void EEPROM::UpdateWord(EEPROM::addr_t addr, uint16_t value) {
    eeprom_update_word((uint16_t *)addr, value);
}

} // namespace eeprom
} // namespace hal
