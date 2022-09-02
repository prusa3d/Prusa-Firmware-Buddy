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
class ConfigurationStore;

template <class T, class CovertTo = T>
class MemConfigItem {
    EepromKey key;

    T data;

    friend ItemUpdater;
    friend ConfigurationStore;

public:
    void init(const T &new_data);

    void set(T new_data);
    T get();
    void dump_data();

    constexpr MemConfigItem(EepromKey key, const T def_val)
        : key(key)
        , data(def_val) {}
};

template <size_t SIZE>
struct MemConfigItem<std::array<char, SIZE>> {
    EepromKey key;
    std::array<char, SIZE> data;
    const char *def_val;
    static constexpr size_t size = SIZE;

    void set_to_default() {
        strcpy(data.data(), def_val);
    }

    void init(const std::array<char, SIZE> &new_data);

    void set(const std::array<char, SIZE> &new_data);
    void set(const char *new_data);
    void dump_data();
    std::array<char, SIZE> get();

    constexpr MemConfigItem(EepromKey key, const char *def_val)
        : key(key)
        , def_val(def_val) {
        set_to_default();
    }
};

}
