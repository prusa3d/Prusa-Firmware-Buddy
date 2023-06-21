#pragma once
#include "journal/backend.hpp"
#include "eeprom_storage.hpp"

namespace eeprom_journal {

/**
 * @brief Bank size available for our backend instance
 *
 */
inline constexpr size_t BANK_SIZE = (8096 - 0x500) / 2;

/**
 * @brief Instance of journal backend with eeprom storage
 *
 */
inline Journal::Backend &backend() {
    return Journal::backend_instance<0x500, BANK_SIZE * 2, EEPROMInstance>();
}

}
