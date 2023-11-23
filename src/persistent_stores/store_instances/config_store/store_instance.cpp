#include "store_instance.hpp"
#include "old_eeprom/last_migration.hpp"

using namespace config_store_ns;

void init_config_store() {
    st25dv64k_init();

    old_eeprom::eeprom_data eeprom_ram_mirror;
    /* Try to read inital data from eeprom multiple times.
     * This is blind fix for random eeprom resets. We know incorrect data can
     * be read without reporting any errors. At last, this happens when read
     * is interrupted by a debugger. This bets on a silent random read error
     * and retries read after failing crc. */
    bool crc_ok = false;
    for (size_t i = 0; i < old_eeprom::EEPROM_DATA_INIT_TRIES && !crc_ok; ++i) {
        old_eeprom::eeprom_init_ram_mirror(eeprom_ram_mirror);
        crc_ok = old_eeprom::eeprom_check_crc32(eeprom_ram_mirror);
    }

    if (!crc_ok) { // old eeprom failed to start, so we're either reset or already in config_store
        config_store().init();
        config_store().load_all();
        const auto journal_state = config_store().get_backend().get_journal_state();
        if (journal_state == journal::Backend::JournalState::ColdStart) {
            config_store_init_result() = config_store_ns::InitResult::cold_start;
        } else {
            config_store_init_result() = config_store_ns::InitResult::normal;
        }
    } else {
        // old eeprom versions migration
        if (old_eeprom::is_older_version(eeprom_ram_mirror)) {
            if (!old_eeprom::eeprom_convert_from(eeprom_ram_mirror)) {
                // nothing was converted and version doesn't match, crc was ok
                // -> weird state
                // -> load defaults (ie start config_store from zero)
                crc_ok = false;
                config_store().get_backend().erase_storage_area(); // guarantee load from nothing
                config_store().init();
                config_store_init_result() = config_store_ns::InitResult::cold_start;
                return;
            }
        }

        // we have valid old eeprom data
        config_store_init_result() = config_store_ns::InitResult::migrated_from_old;
        config_store().get_backend().erase_storage_area(); // guarantee load from nothing
        config_store().init(); // initializes the store's backend, will be a cold start
        config_store().get_backend().override_cold_start_state(); // we don't want the start to be marked as cold to load from our old eeprom transaction from migration
        old_eeprom::migrate(eeprom_ram_mirror.vars.body, config_store().get_backend()); // puts all old values as one transaction into the backend
        config_store().load_all(); // loads the config_store from the one transaction (can trigger further config_store migrations)

        // Since we have at least one migration, bank flip is guaranteed now, which will remove default value journal entries from the old eeprom migration transaction.
    }
}

void init_stores() {
    init_config_store();
}
