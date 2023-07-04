#pragma once

#include "store_definition.hpp"

namespace eeprom_journal {
namespace deprecated_ids {
    inline constexpr uint16_t selftest_result_pre_23[] {
        Journal::hash("Selftest Result"),
    };
} // namespace deprecated_ids

namespace migrations {
    // Commented thoroughly to be used as an example for more migrations.
    void selftest_result_pre_23(Journal::Backend &backend);
} // namespace migrations

/**
 * @brief This array holds previous versions of the configuration store.
 *
 */
inline constexpr Journal::Backend::MigrationFunction migration_functions[] {
    { migrations::selftest_result_pre_23, deprecated_ids::selftest_result_pre_23 },
};

// Span of migration versions to simplify passing it around
inline constexpr std::span<const Journal::Backend::MigrationFunction> migration_functions_span { migration_functions };
} // namespace eeprom_journal
