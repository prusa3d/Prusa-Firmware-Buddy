#pragma once
#include "eeprom_acces.h"
#include "filament.hpp"
#include <utility>
#include <tuple>
#include "configuration_store_structure.hpp"
#include "mem_config_item.hpp"

/**
 * This class is just runtime configuration store which provides easy access to its values.
 * It initializes the values from eeprom and persists them into it on change.
 * Each item it stores can have its own bounds, which is verified when the value is changed
 * Data is written to eeprom only if the value is non-default or if it is overwriting non-default data with default data
 */
template <class CONFIG = ConfigurationStoreStructure>
class ConfigurationStore : public CONFIG {

    EepromAccess<CONFIG::NUM_OF_ITEMS> &eeprom_access;

public:
    template <class T>
    void Set(const char *key, const T &value);
    template <class T>
    std::optional<T> Get(const char *key);

    ConfigurationStore(EepromAccess<CONFIG::NUM_OF_ITEMS> &eepromAccess);
    void Init();
    static ConfigurationStore &GetStore();
};
template <class CONFIG>
template <class T>
void ConfigurationStore<CONFIG>::Set(const char *key, const T &value) {
    eeprom_access.template Set(key, value);
}

template <class CONFIG>
template <class T>
std::optional<T> ConfigurationStore<CONFIG>::Get(const char *key) {
    return eeprom_access.template Get<T>(key);
}

template <class T, class CovertTo>
void MemConfigItem<T, CovertTo>::Set(T new_data) {
    if (new_data != data) {
        if (bound.validate(new_data)) {
            data = new_data;
        } else {
            data = bound.def;
        }
        if (store == nullptr) {
#ifdef EEPROM_UNITTEST
            throw std::invalid_argument("Eeprom item not initialized, add it to tuplify function");
#else
            general_error("Eeprom item not initialized", "Eeprom");
#endif
        }
        store->Set(key, static_cast<CovertTo>(data));
    }
}

template <class T, class CovertTo>
void MemConfigItem<T, CovertTo>::Init(ConfigurationStore<ConfigurationStoreStructure> *configuration_store) {
    store = configuration_store;
    auto new_data = store->Get<CovertTo>(key);
    if (new_data.has_value()) {
        data = static_cast<T>(new_data.value());
    } else {
        data = bound.def;
    }
}

template <class T, size_t SIZE>
void MemConfigItem<std::array<T, SIZE>>::Init(ConfigurationStore<ConfigurationStoreStructure> *configuration_store) {
    store = configuration_store;
    auto new_data = store->template Get<std::array<T, SIZE>>(key);
    if (new_data.has_value()) {
        data = new_data.value();
    } else {
        data.fill(bound.def);
    }
}

template <class T, size_t SIZE>
void MemConfigItem<std::array<T, SIZE>>::Set(std::array<T, SIZE> new_data) {
    if (new_data != data) {
        data = new_data;
        for (auto &elem : data) {
            if (!bound.validate(elem)) {
                elem = bound.def;
            }
        }
        if (store == nullptr) {
#ifdef EEPROM_UNITTEST
            throw std::invalid_argument("Eeprom item not initialized, add it to tuplify function");
#else
            general_error("Eeprom item not initialized", "Eeprom");
#endif
        }
        store->Set(key, data);
    }
}

template <class CONFIG>
void ConfigurationStore<CONFIG>::Init() {
    eeprom_access.Init();
    auto data = CONFIG::tuplify();
    constexpr size_t len = std::tuple_size<decltype(data)>::value;
    // iterate through the whole tuple and initialize the item with default value or value from eeprom
    auto func = [ this, &data ]<size_t... I>(std::index_sequence<I...> sequence) {
        ((
             std::get<I>(data).Init(this)),
            ...);
    };
    func(std::make_index_sequence<len>());
}

template <class CONFIG>
ConfigurationStore<CONFIG> &ConfigurationStore<CONFIG>::GetStore() {
    static EepromAccess<CONFIG::NUM_OF_ITEMS> eeprom_access;
    static ConfigurationStore store { eeprom_access };
    return store;
}
template <class CONFIG>
ConfigurationStore<CONFIG>::ConfigurationStore(EepromAccess<CONFIG::NUM_OF_ITEMS> &eepromAccess)
    : eeprom_access(eepromAccess) {
    static_assert(std::tuple_size<decltype(CONFIG::tuplify())>() == CONFIG::NUM_OF_ITEMS, "Mismatched number of elements member value and elements in tuplify");
}
