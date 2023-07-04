#include "migrations.hpp"

namespace eeprom_journal {
namespace migrations {
    void selftest_result_pre_23(Journal::Backend &backend) {
        // Migrating functions are fired if one of the corresponding deprecated_ids (in this case deprecated_ids::selftest_result_pre_23) is found OR if one of the earlier migration functions had its deprecated ids found.

        // Create helper type to fetch value_types, defaults and hashes in a bit more DRY way
        using SelftestResultPre23T = decltype(DeprecatedStore::selftest_result_pre_23);

        // Create temporaries for each of the deprecated values (in this case just one). Their default values will be used if not found in the journal.
        SelftestResultPre23T::value_type sr_pre23 { SelftestResultPre23T::default_val };

        // Create a callback that will set the desired values when an item is read from journal
        auto callback = [&](Journal::Backend::ItemHeader header, std::array<uint8_t, Journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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

        backend.save_migration_item(Journal::hash("Selftest Result V23"), new_selftest_result); // Save the new data into the backend
    }
} // namespace migrations
} // namespace eeprom_journal
