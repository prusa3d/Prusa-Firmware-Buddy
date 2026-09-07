#include "migrations.hpp"
#include "filament_variant_decode.hpp"
#include <common/utils/algorithm_extensions.hpp>
#include <footer_def.hpp>
#include <footer_eeprom.hpp>
#include <config_store/defaults.hpp>
#include <bsod/bsod.h>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

namespace {

void read_old_item_value_impl(journal::Backend &backend, uint16_t item_hash, void *old_value) {
    auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
        if (header.id == item_hash) {
            memcpy(old_value, buffer.data(), header.len);
        }
    };

    backend.read_items_for_migrations(callback);
}

template <typename OldItem>
auto read_old_item_value(journal::Backend &backend) {
    typename OldItem::value_type old_value = OldItem::default_val;
    read_old_item_value_impl(backend, OldItem::hashed_id, &old_value);
    return old_value;
}
} // namespace

namespace config_store_ns {
namespace migrations {

    void footer_setting_v2(journal::Backend &backend) {
        struct MigrationItem {
            int index;
            uint16_t oldID;
            uint16_t newID;
        };

        std::array migration_mapping = {
            MigrationItem { 0, decltype(DeprecatedStore::footer_setting_0_v2)::hashed_id, journal::hash("Footer Setting 0 v3") },
#if FOOTER_ITEMS_PER_LINE__ > 1
            MigrationItem { 1, decltype(DeprecatedStore::footer_setting_1_v2)::hashed_id, journal::hash("Footer Setting 1 v3") },
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
            MigrationItem { 2, decltype(DeprecatedStore::footer_setting_2_v2)::hashed_id, journal::hash("Footer Setting 2 v3") },
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
            MigrationItem { 3, decltype(DeprecatedStore::footer_setting_3_v2)::hashed_id, journal::hash("Footer Setting 3 v3") },
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
            MigrationItem { 4, decltype(DeprecatedStore::footer_setting_4_v2)::hashed_id, journal::hash("Footer Setting 4 v3") },
#endif
        };

        using Value = decltype(DeprecatedStore::footer_setting_0_v2)::value_type;
        Value values[FOOTER_ITEMS_PER_LINE__] = {
            decltype(DeprecatedStore::footer_setting_0_v2)::default_val,
#if FOOTER_ITEMS_PER_LINE__ > 1
            decltype(DeprecatedStore::footer_setting_1_v2)::default_val,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
            decltype(DeprecatedStore::footer_setting_2_v2)::default_val,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
            decltype(DeprecatedStore::footer_setting_3_v2)::default_val,
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
            decltype(DeprecatedStore::footer_setting_4_v2)::default_val,
#endif
        };

        backend.read_items_for_migrations([&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            for (const auto &migration_rec : migration_mapping) {
                if (header.id == migration_rec.oldID) {
                    memcpy(&values[migration_rec.index], buffer.data(), sizeof(Value));
                    break;
                }
            }
        });

        for (const auto &migration_rec : migration_mapping) {
            backend.save_migration_item<Value>(migration_rec.newID, values[migration_rec.index]);
        }
    }

#if PRINTER_IS_PRUSA_MK4()
    void extended_printer_type(journal::Backend &backend) {
        using OldItem = decltype(DeprecatedStore::xy_motors_400_step);
        bool has_400_motors = true;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == OldItem::hashed_id) {
                memcpy(&has_400_motors, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);

        static_assert(extended_printer_type_model[0] == PrinterModel::mk4);
        static_assert(extended_printer_type_model[2] == PrinterModel::mk3_9);
        backend.save_migration_item<uint8_t>(journal::hash("Extended Printer Type"), has_400_motors ? 0 : 2);
    }
#endif

    void hostname(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::hostname);
        NewItem::value_type hostname { 0 };

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            // Copy either hostname that's not empty
            if ((header.id == decltype(DeprecatedStore::wifi_hostname)::hashed_id || header.id == decltype(DeprecatedStore::lan_hostname)::hashed_id) && strnlen(reinterpret_cast<const char *>(buffer.data()), sizeof(hostname)) != 0) {
                memcpy(&hostname, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);

        if (strlen(hostname.data()) > 0) {
            backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id, hostname);
        }
    }

    void loaded_filament_type(journal::Backend &backend) {
        // See BFW-6236
        using NewItem = decltype(CurrentStore::loaded_filament_type);

        std::array<NewItem::value_type, VirtualToolIndex::count> filament_types;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            const auto ix = stdext::index_of(deprecated_ids::loaded_filament_type, static_cast<uint16_t>(header.id));
            if (ix >= filament_types.size()) {
                return;
            }

            EncodedFilamentType ft;
            debug_assert(header.len == sizeof(ft));
            memcpy(&ft, buffer.data(), sizeof(ft));
            filament_types[ix] = ft;
        };
        backend.read_items_for_migrations(callback);

        for (uint8_t i = 0; i < filament_types.size(); i++) {
            if (filament_types[i] != EncodedFilamentType {}) {
                backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id_first + i, filament_types[i]);
            }
        }
    }

#if HAS_SIDE_LEDS()
    void side_leds_enable(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::side_leds_max_brightness);
        std::optional<NewItem::value_type> val;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == decltype(DeprecatedStore::side_leds_enabled)::hashed_id) {
                val = static_cast<bool>(buffer[0]) ? 255 : 0;
            }
        };
        backend.read_items_for_migrations(callback);

        if (val.has_value()) {
            backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id, *val);
        }
    }
#endif

#if HAS_HOTEND_TYPE_SUPPORT()
    void hotend_type(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::hotend_type);
        using OldItem = decltype(DeprecatedStore::hotend_type_single_hotend);

        OldItem::value_type saved_hotend_type = defaults::hotend_type;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == decltype(DeprecatedStore::hotend_type_single_hotend)::hashed_id) {
                memcpy(&saved_hotend_type, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);

        for (uint8_t i = 0; i < HOTENDS; i++) {
            backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id_first + i, saved_hotend_type);
        }
    }
#endif

#if HAS_CHAMBER_FILTRATION_API() && XL_ENCLOSURE_SUPPORT()
    void xl_enclosure_old_api(journal::Backend &backend) {
        // Old flags
        static constexpr uint8_t ENABLED = 0x01;
        static constexpr uint8_t PRINT_FILTRATION = 0x02;
        static constexpr uint8_t WARNING_SHOWN = 0x04;
        // static constexpr uint8_t EXPIRATION_SHOWN = 0x08;
        static constexpr uint8_t POST_PRINT_FILTRATION = 0x10;
        static constexpr uint8_t REMINDER_5DAYS = 0x20;

        using OldEnabled = decltype(DeprecatedStore::xl_enclosure_flags);
        using NewEnabled = decltype(CurrentStore::xl_enclosure_enabled);
        using NewPrintFiltrationEnabled = decltype(CurrentStore::chamber_post_print_filtration_enable);
        using NewPostPrintFiltrationEnabled = decltype(CurrentStore::chamber_post_print_filtration_enable);
        using NewWarningShown = decltype(CurrentStore::chamber_filter_early_expiration_warning_shown);
        using NewExpirationTimestamp = decltype(CurrentStore::chamber_filter_expiration_postpone_timestamp_1024);

        using OldFilterTimer = decltype(DeprecatedStore::xl_enclosure_filter_timer);
        using NewFilterTimer = decltype(CurrentStore::chamber_filter_time_used_s);

        using OldFanRPM = decltype(DeprecatedStore::xl_enclosure_fan_manual);
        using NewFanRPMPrint = decltype(CurrentStore::chamber_mid_print_filtration_pwm);
        using NewFanRPMPostPrint = decltype(CurrentStore::chamber_post_print_filtration_pwm);

        using OldPostPrintDuration = decltype(DeprecatedStore::xl_enclosure_post_print_duration);
        using NewPostPrintDuration = decltype(CurrentStore::chamber_post_print_filtration_duration_min);

        struct old_variables {
            OldEnabled::value_type saved_flags = OldEnabled::default_val;
            OldFilterTimer::value_type saved_filter_timer = OldFilterTimer::default_val;
            OldFanRPM::value_type saved_fan_manual = OldFilterTimer::default_val;
            OldPostPrintDuration::value_type saved_post_print_dur = OldPostPrintDuration::default_val;
        };

        struct old_variables old_vals;

        auto callback
            = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == OldEnabled::hashed_id) {
                memcpy(&old_vals.saved_flags, buffer.data(), header.len);
            } else if (header.id == OldFilterTimer::hashed_id) {
                memcpy(&old_vals.saved_filter_timer, buffer.data(), header.len);
            } else if (header.id == OldFanRPM::hashed_id) {
                memcpy(&old_vals.saved_fan_manual, buffer.data(), header.len);
            } else if (header.id == OldPostPrintDuration::hashed_id) {
                memcpy(&old_vals.saved_post_print_dur, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);

        backend.save_migration_item<NewFanRPMPrint::value_type>(NewFanRPMPrint::hashed_id, PWM255::from_percent(old_vals.saved_fan_manual));
        backend.save_migration_item<NewFanRPMPostPrint::value_type>(NewFanRPMPostPrint::hashed_id, PWM255::from_percent(old_vals.saved_fan_manual));
        backend.save_migration_item<NewPostPrintDuration::value_type>(NewPostPrintDuration::hashed_id, old_vals.saved_post_print_dur);
        backend.save_migration_item<NewEnabled::value_type>(NewEnabled::hashed_id, old_vals.saved_flags & ENABLED);
        backend.save_migration_item<NewPrintFiltrationEnabled::value_type>(NewPrintFiltrationEnabled::hashed_id, old_vals.saved_flags & PRINT_FILTRATION);
        backend.save_migration_item<NewPostPrintFiltrationEnabled::value_type>(NewPostPrintFiltrationEnabled::hashed_id, old_vals.saved_flags & POST_PRINT_FILTRATION);
        backend.save_migration_item<NewWarningShown::value_type>(NewWarningShown::hashed_id, old_vals.saved_flags & WARNING_SHOWN);

        if (old_vals.saved_flags & REMINDER_5DAYS) {
            // Old implementation saved start of the time period (reminder), new one saves end timestamp
            backend.save_migration_item<NewExpirationTimestamp::value_type>(NewExpirationTimestamp::hashed_id, (old_vals.saved_filter_timer + 5 * 24 * 3600) / 1024);
            backend.save_migration_item<NewFilterTimer::value_type>(NewFilterTimer::hashed_id, 600 * 3600);
        } else {
            // This means saved_filter_timer still holds filter usage time in seconds
            backend.save_migration_item<NewExpirationTimestamp::value_type>(NewExpirationTimestamp::hashed_id, 0);
            backend.save_migration_item<NewFilterTimer::value_type>(NewFilterTimer::hashed_id, old_vals.saved_filter_timer);
        }
    }
#endif

#if HAS_EMERGENCY_STOP()
    void emergency_stop(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::emergency_stop_enable);
        using OldItem = decltype(DeprecatedStore::emergency_stop_enable);

        OldItem::value_type saved_emergency_enable = NewItem::default_val;
        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == OldItem::hashed_id) {
                memcpy(&saved_emergency_enable, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);
        backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id, saved_emergency_enable);
    }
#endif

#if HAS_AUTO_RETRACT()
    void filament_auto_retract(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::filament_retracted_distances);
        using OldItem = decltype(DeprecatedStore::filament_auto_retracted_bitset);

        OldItem::value_type old_byte = OldItem::default_val;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) {
            if (header.id == OldItem::hashed_id) {
                memcpy(&old_byte, buffer.data(), header.len);
            }
        };

        backend.read_items_for_migrations(callback);

        std::bitset<HOTENDS> old_bitset = old_byte;
        static constexpr uint8_t standard_ramming_target_distance = 55;

        for (uint8_t i = 0; i < HOTENDS; i++) {
            backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id_first + i, old_bitset.test(i) ? standard_ramming_target_distance : config_store_ns::invalid_retracted_distance);
        }
    }
#endif

    void filament_order(journal::Backend &backend) {
        using OldItem = decltype(DeprecatedStore::filament_order_v1);
        using NewItem = decltype(CurrentStore::filament_order);

        OldItem::value_type old_data {};
        read_old_item_value_impl(backend, OldItem::hashed_id, &old_data);

        NewItem::value_type new_data {};
        static_assert(old_data.size() == new_data.size() * 2);

        // Old format: std::variant<...> entries, 2 bytes each: [value, discriminant]
        // Represented as array<uint8_t, ...> for manual decoding.
        for (size_t i = 0; i < new_data.size(); i++) {
            const uint8_t value = old_data[i * 2];
            const uint8_t discriminant = old_data[i * 2 + 1];
            new_data[i] = EncodedFilamentType(filament_type_from_variant_bytes(discriminant, value));
        }

        backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id, new_data);
    }

    void printer_setup_done(journal::Backend &backend) {
        const auto old_value = read_old_item_value<decltype(DeprecatedStore::printer_setup_done)>(backend);

        using FirstNewItem = decltype(CurrentStore::printer_hw_config_done);
        backend.save_migration_item<FirstNewItem::value_type>(FirstNewItem::hashed_id, old_value);

        using SecondNewItem = decltype(CurrentStore::printer_network_setup_done);
        backend.save_migration_item<SecondNewItem::value_type>(SecondNewItem::hashed_id, old_value);
    }

    void fsensor_enabled(journal::Backend &backend) {
        const auto old_value = read_old_item_value<decltype(DeprecatedStore::fsensor_enabled_v2)>(backend);

        using NewItem = decltype(CurrentStore::fsensor_enabled);
        backend.save_migration_item<NewItem::value_type>(NewItem::hashed_id, old_value);
    }
} // namespace migrations
} // namespace config_store_ns
