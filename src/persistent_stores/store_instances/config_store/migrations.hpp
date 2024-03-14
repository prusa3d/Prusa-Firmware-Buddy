#pragma once

#include "store_definition.hpp"
#include <printers.h>

namespace config_store_ns {
namespace deprecated_ids {
    inline constexpr uint16_t selftest_result_pre_23[] {
        journal::hash("Selftest Result"),
    };
    inline constexpr uint16_t footer_setting_v1[] {
        journal::hash("Footer Setting"),
    };
    inline constexpr uint16_t selftest_result_pre_gears[] {
        journal::hash("Selftest Result V23"),
    };
    inline constexpr uint16_t fsensor_enabled_v1[] {
        journal::hash("FSensor Enabled"),
    };
} // namespace deprecated_ids

namespace migrations {
    // Commented thoroughly to be used as an example for more migrations.
    void selftest_result_pre_23(journal::Backend &backend);
#if PRINTER_IS_PRUSA_XL // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
    void footer_setting_v1(journal::Backend &backend);
#endif
    void selftest_result_pre_gears(journal::Backend &backend);
    void fsensor_enabled_v1(journal::Backend &backend);
} // namespace migrations

/**
 * @brief This array holds previous versions of the configuration store.
 *
 */
inline constexpr journal::Backend::MigrationFunction migration_functions[] {
    { migrations::selftest_result_pre_23, deprecated_ids::selftest_result_pre_23 },
#if PRINTER_IS_PRUSA_XL // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
        { migrations::footer_setting_v1, deprecated_ids::footer_setting_v1 },
#endif
        { migrations::selftest_result_pre_gears, deprecated_ids::selftest_result_pre_gears },
        { migrations::fsensor_enabled_v1, deprecated_ids::fsensor_enabled_v1 },
};

// Span of migration versions to simplify passing it around
inline constexpr std::span<const journal::Backend::MigrationFunction> migration_functions_span { migration_functions };
} // namespace config_store_ns
