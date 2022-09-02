#include "configuration_store.hpp"
#include "configuration_store.h"
#include "eeprom.h"
#include "sys.h"
LOG_COMPONENT_DEF(ConfigurationStoreLog, LOG_SEVERITY_DEBUG);
using namespace configuration_store;

ConfigurationStore &config_store() {
    return ConfigurationStore::GetStore();
}
FreeRTOS_Mutex &configuration_store::get_item_mutex() {
    static FreeRTOS_Mutex mutex;
    return mutex;
}

void ConfigurationStore::init() {
    ItemUpdater updater(*this);
    std::visit([&](auto &access) { access.init(updater); },
        backend);
    updater.migrate();

    auto eeprom_init_state = eeprom_init();
    switch (eeprom_init_state) {
    case EEPROM_INIT_Defaults:
        //we have already migrated to configuration store
        break;
    case EEPROM_INIT_Upgraded:
    case EEPROM_INIT_Normal:
        //migrate old eeprom, invalidate it and reset the system to initialize FW with valid data
        migrate_from_old_eeprom();
        eeprom_clear();
        sys_reset();
        break;
    default:
        break;
    }
}

ConfigurationStore &ConfigurationStore::GetStore() {
    static ConfigurationStore store(EepromAccess {});
    return store;
}

/**
 * @brief sets values of items to their default value and invalidates data in eeprom
 * @tparam CONFIG
 */
void ConfigurationStore::factory_reset() {
    // just invalidate the eeprom and then reset the printer from outside this function
    // we need to reset the printer to load default values
    std::unique_lock lock(get_item_mutex());
    std::visit([&](auto &access) { access.reset(); },
        backend);
}
void ConfigurationStore::migrate_from_old_eeprom() {
}
