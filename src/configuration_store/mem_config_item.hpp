#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <array>
#include <tuple>
#include <optional>
#include "item_updater.hpp"
#include <string.h>

namespace configuration_store {
class ItemUpdater;

using EepromKey = const char *;
struct ConfigurationStoreStructure;
template <class T = ConfigurationStoreStructure>
class ConfigurationStore;

template <class T, class CovertTo = T>
class MemConfigItem {
    EepromKey key;

    T data;
    T def_val;

    void set_to_default() {
        data = def_val;
    }
    friend ItemUpdater;
    friend ConfigurationStore<>;

public:
    void init(const T &new_data);

    void set(T new_data);
    T get();

    constexpr MemConfigItem(EepromKey key, const T def_val)
        : key(key)
        , data(def_val)
        , def_val(def_val) {}
};

template <class T, size_t SIZE>
struct MemConfigItem<std::array<T, SIZE>> {
    EepromKey key;

    std::array<T, SIZE> data;
    T def_val;

    void set_to_default() {
        data.fill(def_val);
    }

    void init(const std::array<T, SIZE> &new_data);

    void set(const std::array<T, SIZE> &new_data);

    std::array<T, SIZE> get();

    constexpr MemConfigItem(EepromKey key, const T def_val)
        : key(key)
        , def_val(def_val) {
        data.fill(def_val);
    }
};

template <size_t SIZE>
struct MemConfigItem<std::array<char, SIZE>> {
    EepromKey key;
    std::array<char, SIZE> data;
    const char *def_val;

    void set_to_default() {
        strcpy(data.data(), def_val);
    }

    void init(const std::array<char, SIZE> &new_data);

    void set(const std::array<char, SIZE> &new_data);
    void set(const char *new_data);
    std::array<char, SIZE> get();

    constexpr MemConfigItem(EepromKey key, const char *def_val)
        : key(key)
        , def_val(def_val) {
        set_to_default();
    }
};

}
