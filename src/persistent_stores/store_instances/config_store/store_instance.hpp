#pragma once

#include <journal/store.hpp>
#include "store_definition.hpp"
#include "migrations.hpp"

namespace config_store_ns {

enum class InitResult {
    migrated_from_old,
    cold_start,
    normal,
    not_yet_init
};

} // namespace config_store_ns

/**
 * @brief Instance of ConfigStore journal with journal strategy and EEPROM backend. Currently there is only one config store in the entire project, hence why the 'simple' "config_store" name
 *
 */
inline decltype(auto) config_store() {
    return journal::store<config_store_ns::CurrentStore, config_store_ns::DeprecatedStore, config_store_ns::migration_functions_span>();
}

// has to be done this way because it's used before global constructors are run
inline config_store_ns::InitResult &config_store_init_result() {
    static config_store_ns::InitResult init_result { config_store_ns::InitResult::not_yet_init };
    return init_result;
}

void init_config_store();
