/// @file
#pragma once

#include <option/has_config_store_wo_backend.h>
#if HAS_CONFIG_STORE_WO_BACKEND()
    #include <no_backend/store.hpp>
#else
    #include <journal/store.hpp>
    #include "migrations.hpp"
#endif
#include "store_definition.hpp"

#if HAS_CONFIG_STORE_WO_BACKEND()
using ConfigStore = no_backend::Store<config_store_ns::CurrentStore, config_store_ns::DeprecatedStore>;
#else
using ConfigStore = journal::Store<config_store_ns::CurrentStore, config_store_ns::DeprecatedStore, config_store_ns::migration_functions_span>;
#endif

/// The full config store, including store-level operations.
/// This one is heavy. Use config_store() if possible.
ConfigStore &config_store_journal();
