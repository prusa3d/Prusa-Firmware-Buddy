#pragma once

#include "store_definition.hpp"
#include <printers.h>
#include <option/has_selftest.h>
#include <option/has_gui.h>

namespace config_store_ns {
namespace deprecated_ids {
    inline constexpr uint16_t selftest_result_pre_23[] {
        journal::hash("Selftest Result"),
    };
    inline constexpr uint16_t footer_setting_v1[] {
        journal::hash("Footer Setting"),
    };
    inline constexpr uint16_t footer_setting_v2[] {
        decltype(DeprecatedStore::footer_setting_0_v2)::hashed_id,
#if FOOTER_ITEMS_PER_LINE__ > 1
            decltype(DeprecatedStore::footer_setting_1_v2)::hashed_id,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
            decltype(DeprecatedStore::footer_setting_2_v2)::hashed_id,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
            decltype(DeprecatedStore::footer_setting_3_v2)::hashed_id,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
            decltype(DeprecatedStore::footer_setting_4_v2)::hashed_id,
#endif
    };
    inline constexpr uint16_t selftest_result_pre_gears[] {
        journal::hash("Selftest Result V23"),
    };
    inline constexpr uint16_t fsensor_enabled_v1[] {
        journal::hash("FSensor Enabled"),
    };
#if PRINTER_IS_PRUSA_MK4()
    inline constexpr uint16_t extended_printer_type[] {
        journal::hash("400 step motors on X and Y axis"),
    };
#endif
    inline constexpr uint16_t hostname[] {
        decltype(DeprecatedStore::lan_hostname)::hashed_id,
        decltype(DeprecatedStore::wifi_hostname)::hashed_id,
    };
} // namespace deprecated_ids

namespace migrations {
    // Commented thoroughly to be used as an example for more migrations.
    void selftest_result_pre_23(journal::Backend &backend);

#if PRINTER_IS_PRUSA_XL() // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
    void footer_setting_v1(journal::Backend &backend);
#endif
    void footer_setting_v2(journal::Backend &backend);

    void selftest_result_pre_gears(journal::Backend &backend);
    void fsensor_enabled_v1(journal::Backend &backend);

#if PRINTER_IS_PRUSA_MK4()
    void extended_printer_type(journal::Backend &backend);
#endif

    void hostname(journal::Backend &backend);
} // namespace migrations

/**
 * @brief This array holds previous versions of the configuration store.
 *
 */
inline constexpr journal::Backend::MigrationFunction migration_functions[] {
#if HAS_SELFTEST()
    { migrations::selftest_result_pre_23, deprecated_ids::selftest_result_pre_23 },
#endif
#if PRINTER_IS_PRUSA_XL() && HAS_GUI() // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
        { migrations::footer_setting_v1, deprecated_ids::footer_setting_v1 },
#endif
#if HAS_SELFTEST()
        { migrations::selftest_result_pre_gears, deprecated_ids::selftest_result_pre_gears },
#endif
        { migrations::fsensor_enabled_v1, deprecated_ids::fsensor_enabled_v1 },
        { migrations::footer_setting_v2, deprecated_ids::footer_setting_v2 },

#if PRINTER_IS_PRUSA_MK4()
        { migrations::extended_printer_type, deprecated_ids::extended_printer_type },
#endif

        { migrations::hostname, deprecated_ids::hostname },
};

// Span of migration versions to simplify passing it around
inline constexpr std::span<const journal::Backend::MigrationFunction> migration_functions_span { migration_functions };
} // namespace config_store_ns
