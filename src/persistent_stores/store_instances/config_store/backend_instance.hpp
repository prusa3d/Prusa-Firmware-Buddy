#pragma once
#include "journal/backend.hpp"
#include <storage_drivers/eeprom_storage.hpp>

namespace config_store_ns {

inline constexpr size_t start_address { 0x500 };

/**
 * @brief Bank size available for our backend instance
 *
 */
inline constexpr size_t BANK_SIZE = (8096 - start_address) / 2;

/**
 * @brief Instance of journal backend with eeprom storage
 *
 */
inline journal::Backend &backend() {
    return journal::backend_instance<start_address, BANK_SIZE * 2, EEPROMInstance>();
}

} // namespace config_store_ns
