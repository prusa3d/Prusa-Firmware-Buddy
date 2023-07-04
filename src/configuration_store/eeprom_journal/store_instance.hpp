#pragma once

#include <journal/configuration_store.hpp>
#include "store_definition.hpp"
#include "migrations.hpp"

namespace eeprom_journal {

enum class InitResult {
    migrated_from_old,
    cold_start,
    normal,
    not_yet_init
};

} // namespace eeprom_journal

/**
 * @brief Instance of ConfigStore journal with journal strategy and EEPROM backend. Currently there is only one config store in the entire project, hence why the 'simple' "config_store" name
 *
 */
inline decltype(auto) config_store() {
    return Journal::journal_store<eeprom_journal::CurrentStore, eeprom_journal::DeprecatedStore, eeprom_journal::migration_functions_span>();
}

// has to be done this way because it's used before global constructors are run
inline eeprom_journal::InitResult &config_store_init_result() {
    static eeprom_journal::InitResult init_result { eeprom_journal::InitResult::not_yet_init };
    return init_result;
}
