#include "migrations.hpp"
#include <common/utils/algorithm_extensions.hpp>
#include <footer_def.hpp>
#include <footer_eeprom.hpp>
#include <config_store/defaults.hpp>
#if HAS_AUTO_RETRACT()
    #include <feature/auto_retract/auto_retract.hpp>
#endif

namespace config_store_ns {
namespace migrations {
#if HAS_SELFTEST()
    void selftest_result_pre_23(journal::Backend &backend) {
        // Migrating functions are fired if one of the corresponding deprecated_ids (in this case deprecated_ids::selftest_result_pre_23) is found OR if one of the earlier migration functions had its deprecated ids found.

        // Create helper type to fetch value_types, defaults and hashes in a bit more DRY way
        using SelftestResultPre23T = decltype(DeprecatedStore::selftest_result_pre_23);

        // Create temporaries for each of the deprecated values (in this case just one). Their default values will be used if not found in the journal.
        SelftestResultPre23T::value_type sr_pre23 { SelftestResultPre23T::default_val };

        // Create a callback that will set the desired values when an item is read from journal
        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == SelftestResultPre23T::hashed_id) {
                memcpy(&sr_pre23, buffer.data(), header.len);
            }
        };
        // Check the backend for the wanted items. This will search both current bank and intermediary transactions in next bank.
        backend.read_items_for_migrations(callback);

        // At this point temporaries are properly filled with values found in the journal.
        // Now is the time for data manipulation and then saving into backend whatever is desired

        // Create new data from the temporaries
        SelftestResult new_selftest_result { sr_pre23 };

        backend.save_migration_item<SelftestResult>(journal::hash("Selftest Result V23"), new_selftest_result); // Save the new data into the backend
    }
#endif

#if PRINTER_IS_PRUSA_XL() and HAS_GUI() // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
    void footer_setting_v1(journal::Backend &backend) {
        // See selftest_result_pre_23 (above) for in-depth commentary
        using FooterSettingsV1 = decltype(DeprecatedStore::footer_setting_v1);
        FooterSettingsV1::value_type footer_setting_v1 { FooterSettingsV1::default_val };
        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == FooterSettingsV1::hashed_id) {
                memcpy(&footer_setting_v1, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);

        auto decoded_rec { footer::eeprom::decode_from_old_eeprom_v22(footer_setting_v1) };

        backend.save_migration_item<footer::Item>(journal::hash("Footer Setting 0"), decoded_rec[0]);
    #if FOOTER_ITEMS_PER_LINE__ > 1
        backend.save_migration_item<footer::Item>(journal::hash("Footer Setting 1"), decoded_rec[1]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 2
        backend.save_migration_item<footer::Item>(journal::hash("Footer Setting 2"), decoded_rec[2]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 3
        backend.save_migration_item<footer::Item>(journal::hash("Footer Setting 3"), decoded_rec[3]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 4
        backend.save_migration_item<footer::Item>(journal::hash("Footer Setting 4"), decoded_rec[4]);
    #endif
    }
#endif

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

        backend.read_items_for_migrations([&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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

#if HAS_SELFTEST()
    void selftest_result_pre_gears(journal::Backend &backend) {
        // See selftest_result_pre_23 (above) for in-depth commentary
        using SelftestResultPreGearsT = decltype(DeprecatedStore::selftest_result_pre_gears);
        SelftestResultPreGearsT::value_type sr_pre_gears { SelftestResultPreGearsT::default_val };
        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == SelftestResultPreGearsT::hashed_id) {
                memcpy(&sr_pre_gears, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);
        SelftestResult new_selftest_result { sr_pre_gears };
        backend.save_migration_item<SelftestResult>(journal::hash("Selftest Result Gears"), new_selftest_result);
    }
#endif

    void fsensor_enabled_v1(journal::Backend &backend) {
        // See selftest_result_pre_23 (above) for in-depth commentary
        using FSensorEnabledV1T = decltype(DeprecatedStore::fsensor_enabled_v1);
        FSensorEnabledV1T::value_type fs_enabled_v1 { FSensorEnabledV1T::default_val };
        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == FSensorEnabledV1T::hashed_id) {
                memcpy(&fs_enabled_v1, buffer.data(), header.len);
            }
        };
        backend.read_items_for_migrations(callback);
        bool new_fs_enabled { fs_enabled_v1 };
        backend.save_migration_item<bool>(journal::hash("FSensor Enabled V2"), new_fs_enabled);
    }

#if PRINTER_IS_PRUSA_MK4()
    void extended_printer_type(journal::Backend &backend) {
        // See selftest_result_pre_23 (above) for in-depth commentary
        using OldItem = decltype(DeprecatedStore::xy_motors_400_step);
        bool has_400_motors = true;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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
        // See selftest_result_pre_23 (above) for in-depth commentary
        using NewItem = decltype(CurrentStore::hostname);
        NewItem::value_type hostname { 0 };

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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

        std::array<NewItem::value_type, EXTRUDERS> filament_types;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            const auto ix = stdext::index_of(deprecated_ids::loaded_filament_type, static_cast<uint16_t>(header.id));
            if (ix >= filament_types.size()) {
                return;
            }

            EncodedFilamentType ft;
            assert(header.len == sizeof(ft));
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
    void side_leds_dimming(journal::Backend &backend) {
        // The old side_leds_dimming_enabled=always mode dimmed LEDs even during printing.
        // The new model uses a fixed side_leds_print_brightness (default 255 = full).
        // Preserve intent: if the user had 'always' dimming, copy dimmed_brightness into
        // print_brightness so prints are not unexpectedly at full brightness after upgrade.
        using OldDimming = decltype(DeprecatedStore::side_leds_dimming_enabled);
        using DimmedBrightness = decltype(CurrentStore::side_leds_dimmed_brightness);
        using PrintBrightness = decltype(CurrentStore::side_leds_print_brightness);

        OldDimming::value_type dimming_mode { OldDimming::default_val };
        DimmedBrightness::value_type dimmed_brightness { DimmedBrightness::default_val };

        backend.read_items_for_migrations([&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == OldDimming::hashed_id) {
                memcpy(&dimming_mode, buffer.data(), header.len);
            } else if (header.id == DimmedBrightness::hashed_id) {
                memcpy(&dimmed_brightness, buffer.data(), header.len);
            }
        });

        if (dimming_mode == leds::DimmingEnabled::always) {
            backend.save_migration_item<PrintBrightness::value_type>(PrintBrightness::hashed_id, dimmed_brightness);
        }
    }

    void side_leds_enable(journal::Backend &backend) {
        using NewItem = decltype(CurrentStore::side_leds_max_brightness);
        std::optional<NewItem::value_type> val;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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
            = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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
        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) {
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

    void printer_setup_done(journal::Backend &backend) {
        using FirstNewItem = decltype(CurrentStore::printer_hw_config_done);
        using SecondNewItem = decltype(CurrentStore::printer_network_setup_done);
        using OldItem = decltype(DeprecatedStore::printer_setup_done);

        OldItem::value_type old_value = OldItem::default_val;

        auto callback = [&](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
            if (header.id == OldItem::hashed_id) {
                memcpy(&old_value, buffer.data(), header.len);
            }
        };

        backend.read_items_for_migrations(callback);

        backend.save_migration_item<FirstNewItem::value_type>(FirstNewItem::hashed_id, old_value);
        backend.save_migration_item<SecondNewItem::value_type>(SecondNewItem::hashed_id, old_value);
    }
} // namespace migrations
} // namespace config_store_ns
