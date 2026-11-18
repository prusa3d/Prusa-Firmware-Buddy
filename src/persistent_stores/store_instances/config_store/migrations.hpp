#pragma once

#include "store_definition.hpp"
#include <printers.h>
#include <option/has_side_leds.h>
#include <option/has_chamber_filtration_api.h>
#include <option/xl_enclosure_support.h>
#include <option/has_emergency_stop.h>
#include <option/has_auto_retract.h>

namespace config_store_ns {
namespace deprecated_ids {
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
#if PRINTER_IS_PRUSA_MK4()
    inline constexpr uint16_t extended_printer_type[] {
        journal::hash("400 step motors on X and Y axis"),
    };
#endif
    inline constexpr uint16_t hostname[] {
        decltype(DeprecatedStore::lan_hostname)::hashed_id,
        decltype(DeprecatedStore::wifi_hostname)::hashed_id,
    };
    inline constexpr std::array loaded_filament_type {
        decltype(DeprecatedStore::filament_type_0)::hashed_id,
#if EXTRUDERS > 1
            decltype(DeprecatedStore::filament_type_1)::hashed_id,
            decltype(DeprecatedStore::filament_type_2)::hashed_id,
            decltype(DeprecatedStore::filament_type_3)::hashed_id,
            decltype(DeprecatedStore::filament_type_4)::hashed_id,
            decltype(DeprecatedStore::filament_type_5)::hashed_id,
#endif
    };
#if HAS_SIDE_LEDS()
    inline constexpr uint16_t side_leds_enable[] {
        decltype(DeprecatedStore::side_leds_enabled)::hashed_id,
    };
#endif
#if HAS_HOTEND_TYPE_SUPPORT()
    inline constexpr uint16_t hotend_type_single_hotend[] {
        decltype(DeprecatedStore::hotend_type_single_hotend)::hashed_id,
    };
#endif

#if HAS_CHAMBER_FILTRATION_API() && XL_ENCLOSURE_SUPPORT()
    inline constexpr uint16_t xl_enclosure_old_api_ids[] {
        decltype(DeprecatedStore::xl_enclosure_flags)::hashed_id,
        decltype(DeprecatedStore::xl_enclosure_fan_manual)::hashed_id,
        decltype(DeprecatedStore::xl_enclosure_filter_timer)::hashed_id,
        decltype(DeprecatedStore::xl_enclosure_post_print_duration)::hashed_id,
        decltype(DeprecatedStore::xl_enclosure_fan_manual)::hashed_id,
        decltype(DeprecatedStore::xl_enclosure_post_print_duration)::hashed_id,
    };
#endif

#if HAS_EMERGENCY_STOP()
    inline constexpr uint16_t emergency_stop_enable[] {
        decltype(DeprecatedStore::emergency_stop_enable)::hashed_id,
    };
#endif

#if HAS_AUTO_RETRACT()
    inline constexpr uint16_t filament_auto_retracted_bitset[] {
        decltype(DeprecatedStore::filament_auto_retracted_bitset)::hashed_id,
    };
#endif
    inline constexpr uint16_t printer_setup_done[] {
        decltype(DeprecatedStore::printer_setup_done)::hashed_id,
    };
    inline constexpr uint16_t fsensor_enabled[] {
        decltype(DeprecatedStore::fsensor_enabled_v2)::hashed_id,
    };
    inline constexpr uint16_t filament_order_v1[] {
        decltype(DeprecatedStore::filament_order_v1)::hashed_id,
    };
    inline constexpr uint16_t connect_host[] {
        decltype(DeprecatedStore::connect_host_v1)::hashed_id,
    };
} // namespace deprecated_ids

namespace migrations {

    void footer_setting_v2(journal::Backend &backend);

#if PRINTER_IS_PRUSA_MK4()
    void extended_printer_type(journal::Backend &backend);
#endif

    void hostname(journal::Backend &backend);
    void loaded_filament_type(journal::Backend &backend);

#if HAS_SIDE_LEDS()
    void side_leds_enable(journal::Backend &backend);
#endif
    void hotend_type(journal::Backend &backend);

#if HAS_CHAMBER_FILTRATION_API() && XL_ENCLOSURE_SUPPORT()
    void xl_enclosure_old_api(journal::Backend &backend);
#endif

#if HAS_EMERGENCY_STOP()
    void
    emergency_stop(journal::Backend &backend);
#endif
    void printer_setup_done(journal::Backend &backend);

    void fsensor_enabled(journal::Backend &backend);

    void filament_order(journal::Backend &backend);

    void connect_host(journal::Backend &backend);
} // namespace migrations

/**
 * @brief This array holds previous versions of the configuration store.
 *
 */
inline constexpr journal::Backend::MigrationFunction migration_functions[] {
    { migrations::footer_setting_v2, deprecated_ids::footer_setting_v2 },

#if PRINTER_IS_PRUSA_MK4()
        { migrations::extended_printer_type, deprecated_ids::extended_printer_type },
#endif

        { migrations::hostname, deprecated_ids::hostname },
        { migrations::loaded_filament_type, deprecated_ids::loaded_filament_type },
#if HAS_SIDE_LEDS()
        { migrations::side_leds_enable, deprecated_ids::side_leds_enable },
#endif
#if HAS_HOTEND_TYPE_SUPPORT()
        { migrations::hotend_type, deprecated_ids::hotend_type_single_hotend },

#endif
#if HAS_CHAMBER_FILTRATION_API() && XL_ENCLOSURE_SUPPORT()
        { migrations::xl_enclosure_old_api, deprecated_ids::xl_enclosure_old_api_ids },
#endif
#if HAS_EMERGENCY_STOP()
        { migrations::emergency_stop, deprecated_ids::emergency_stop_enable },
#endif
        { migrations::filament_order, deprecated_ids::filament_order_v1 },
        { migrations::printer_setup_done, deprecated_ids::printer_setup_done },
        { migrations::fsensor_enabled, deprecated_ids::fsensor_enabled },
        { migrations::connect_host, deprecated_ids::connect_host },
};

// Span of migration versions to simplify passing it around
inline constexpr std::span<const journal::Backend::MigrationFunction> migration_functions_span { migration_functions };
} // namespace config_store_ns
