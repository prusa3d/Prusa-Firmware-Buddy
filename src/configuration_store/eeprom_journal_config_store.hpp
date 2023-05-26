#pragma once
#include "eeprom_journal.hpp"
#include "journal/configuration_store.hpp"

namespace journal_config_store {
/**
 * This struct defines the structure of configuration store. To use it just define new StoreItem and then you can access it via config_store function
 */
struct EEPROMJournalConfig : public Journal::CurrentStoreConfig<Journal::Backend, EEPROM_journal> {
    StoreItem<bool, false, Journal::hash("Fan check")> fan_check;
};
static_assert(sizeof(EEPROMJournalConfig) < (BANK_SIZE / 100) * 75, "EEPROM bank is almost full");

/**
 * This struct defines what should be done with deprecated items.
 * To define what item it should be migrated to use member pointer to that item, the pointer can point to other deprecated item or valid item.
 * The item it is migrated to has to have constructor which accepts the previous item
 */
struct DeprecatedEEPROMJournalItems : public Journal::DeprecatedStoreConfig<Journal::Backend> {
    //        DeprecatedStoreItem<bool,true, Journal::hash("Fan check"),&EEPROMJournalConfig::fan_check> fan_check;
};

}
inline Journal::ConfigStore<journal_config_store::EEPROMJournalConfig, journal_config_store::DeprecatedEEPROMJournalItems> &config_store() {
    return Journal::journal<journal_config_store::EEPROMJournalConfig, journal_config_store::DeprecatedEEPROMJournalItems>();
}
