/// @file eeprom.cpp
#include "../eeprom.h"
#include <avr/eeprom.h>

namespace hal {
namespace eeprom {

EEPROM eeprom;

uint8_t EEPROM::ReadByte(EEPROM::addr_t addr) {
    return eeprom_read_byte((const uint8_t *)addr);
}

void EEPROM::UpdateByte(EEPROM::addr_t addr, uint8_t value) {
    eeprom_update_byte((uint8_t *)addr, value);
}

} // namespace eeprom
} // namespace hal
