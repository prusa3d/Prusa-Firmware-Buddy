#pragma once
#include "eeprom_access.hpp"
#include "filament.hpp"
#include <utility>
#include <tuple>
#include "configuration_store_structure.hpp"
#include "item_updater.hpp"
namespace configuration_store {
/**
 * This class is configuration store which provides easy access to its values.
 * It initializes the values from eeprom and persists them into it on change.
 * Data is written to eeprom only if the value is non-default or if it is overwriting non-default data with default data
 *
 * To add new item to configuration store, you need to add it to json from which it is generated
 *
 * The values are stored in RAM so reads does not read data from the eeprom.
 *
 * To access the values get singleton instance of the store and access the items like members of any other class. To change the items use their Set and Get methods.
 */
template <class CONFIG>
class ConfigurationStore : public CONFIG {
    friend ItemUpdater;

public:
    ConfigurationStore() = default;
    ConfigurationStore(const ConfigurationStore &other) = delete;
    void init();
    static ConfigurationStore &GetStore();
    void factory_reset();
};

template <class CONFIG>
void ConfigurationStore<CONFIG>::init() {
    ItemUpdater updater(*this);
    EepromAccess::instance().init(updater);
}

template <class CONFIG>
ConfigurationStore<CONFIG> &ConfigurationStore<CONFIG>::GetStore() {
    static ConfigurationStore store {};
    return store;
}

/**
 * @brief sets values of items to their default value and invalidates data in eeprom
 * @tparam CONFIG
 */
template <class CONFIG>
void ConfigurationStore<CONFIG>::factory_reset() {

    std::apply([&](auto &... items) {
        ((items.set_to_default()), ...);
    },
        CONFIG::tuplify());
    EepromAccess::instance().reset();
}
}
configuration_store::ConfigurationStore<configuration_store::ConfigurationStoreStructure> &config_store();
