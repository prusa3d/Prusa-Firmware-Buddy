/// @file
#pragma once

#include "store_definition.hpp"

namespace config_store_ns {

enum class InitResult {
    cold_start,
    normal,
    not_yet_init
};

} // namespace config_store_ns

/// Lightweight access to the items of the config store.
/// Use this unless you absolutely need config_store_journal()
///
/// If you want to read values from the config store in gdb, do:
/// p 'config_store_journal()::instance'.item_name
config_store_ns::CurrentStore &config_store();

// has to be done this way because it's used before global constructors are run
inline config_store_ns::InitResult &config_store_init_result() {
    static config_store_ns::InitResult init_result { config_store_ns::InitResult::not_yet_init };
    return init_result;
}

void init_config_store();
