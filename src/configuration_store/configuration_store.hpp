#pragma once
#include "eeprom_access.hpp"
#include "filament.hpp"
#include <utility>
#include <tuple>
#include "configuration_store_structure.hpp"
#include "item_updater.hpp"
#include "mem_config_item.hpp"
#include <variant>

#ifndef EEPROM_UNITTEST
    #include "disable_interrupts.h"
#endif

LOG_COMPONENT_REF(ConfigurationStoreLog);
namespace configuration_store {
using Backend = std::variant<EepromAccess>;

FreeRTOS_Mutex &get_item_mutex();
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
class ConfigurationStore : public ConfigurationStoreStructure {
#ifdef EEPROM_UNITTEST
public:
#endif
    Backend backend;
    void migrate_from_old_eeprom();

public:
    template <class T>
    void set(const char *key, const T &data) {
        std::visit(
            [&](auto &access) { access.template set(key, data); },
            backend);
    };

    ConfigurationStore(Backend &&backend)
        : backend(std::move(backend)) {};
    ConfigurationStore(const ConfigurationStore &other) = delete;
    void init();
    static ConfigurationStore &GetStore();
    void dump_data(Backend data_dump);
    void factory_reset();
};

template <class T, class CovertTo>
void MemConfigItem<T, CovertTo>::set(T new_data) {
    std::unique_lock<FreeRTOS_Mutex> lock(get_item_mutex());
    if (!(data == new_data)) {
        buddy::DisableInterrupts disable;
        data = new_data;
        // using singleton to save on RAM usage, items does not need to have pointer
        ConfigurationStore::GetStore().template set(key, data);
    }
}
template <class T, class CovertTo>
T MemConfigItem<T, CovertTo>::get() {
    buddy::DisableInterrupts disable;
    return data;
}

template <class T, class CovertTo>
void MemConfigItem<T, CovertTo>::init(const T &new_data) {
    log_debug(ConfigurationStoreLog, "Initialized item: %s", key);
    data = new_data;
}

template <class T, class CovertTo>
void MemConfigItem<T, CovertTo>::dump_data() {
    // using singleton to save on RAM usage, items does not need to have pointer
    ConfigurationStore::GetStore().template set(key, data);
}
template <size_t SIZE>
void MemConfigItem<std::array<char, SIZE>>::set(const char *new_data) {
    if (strcmp((char *)(data.data()), new_data) != 0) {
        std::unique_lock<FreeRTOS_Mutex> lock(get_item_mutex());
        {
            buddy::DisableInterrupts disable;
            strncpy(data.data(), new_data, SIZE);
        }
        // using singleton to save on RAM usage, items does not need to have pointer
        ConfigurationStore::GetStore().template set(key, data);
    }
}
template <size_t SIZE>
void MemConfigItem<std::array<char, SIZE>>::set(const std::array<char, SIZE> &new_data) {
    if (data != new_data) {
        std::unique_lock<FreeRTOS_Mutex> lock(get_item_mutex());
        {
            buddy::DisableInterrupts disable;
            data = new_data;
        }
        // using singleton to save on RAM usage, items does not need to have pointer
        ConfigurationStore::GetStore().template set(key, data);
    }
}
template <size_t SIZE>
std::array<char, SIZE> MemConfigItem<std::array<char, SIZE>>::get() {
    buddy::DisableInterrupts disable;
    return data;
}

template <size_t SIZE>
void MemConfigItem<std::array<char, SIZE>>::init(const std::array<char, SIZE> &new_data) {
    log_debug(ConfigurationStoreLog, "Initialized item: %s", key);
    data = new_data;
}
template <size_t SIZE>
void MemConfigItem<std::array<char, SIZE>>::dump_data() {
    // using singleton to save on RAM usage, items does not need to have pointer
    ConfigurationStore::GetStore().template set(key, data);
}
}
configuration_store::ConfigurationStore &config_store();
