#pragma once
#include "journal/backend.hpp"
#include "eeprom_storage.hpp"

inline constexpr size_t BANK_SIZE = (8096 - 0x500) / 2;
inline Journal::Backend &EEPROM_journal() {
    return Journal::backend_instance<0x500, BANK_SIZE * 2, EEPROMInstance>();
}
