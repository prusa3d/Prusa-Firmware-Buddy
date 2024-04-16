#include "migrations.hpp"
#include <footer_def.hpp>
#include <footer_eeprom.hpp>

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

        backend.save_migration_item(journal::hash("Selftest Result V23"), new_selftest_result); // Save the new data into the backend
    }
#endif

#if PRINTER_IS_PRUSA_XL and HAS_GUI() // MINI goes directly from old eeprom to multiple footer items, MK4 gets its footer reset
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

        backend.save_migration_item(journal::hash("Footer Setting 0"), decoded_rec[0]);
    #if FOOTER_ITEMS_PER_LINE__ > 1
        backend.save_migration_item(journal::hash("Footer Setting 1"), decoded_rec[1]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 2
        backend.save_migration_item(journal::hash("Footer Setting 2"), decoded_rec[2]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 3
        backend.save_migration_item(journal::hash("Footer Setting 3"), decoded_rec[3]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 4
        backend.save_migration_item(journal::hash("Footer Setting 4"), decoded_rec[4]);
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
            backend.save_migration_item(migration_rec.newID, values[migration_rec.index]);
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
        backend.save_migration_item(journal::hash("Selftest Result Gears"), new_selftest_result);
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
        backend.save_migration_item(journal::hash("FSensor Enabled V2"), new_fs_enabled);
    }
} // namespace migrations
} // namespace config_store_ns
