#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <array>
#include <utility>
using EepromKey = const char *;
struct ConfigurationStoreStructure;
template <class T>
class ConfigurationStore;

template <class T>
struct MemConfigBound {
    T min;
    T max;
    T def;
    bool validate(const T &data) const {
        return (data >= min && data <= max);
    }
};

template <class T, class CovertTo = T>
struct MemConfigItem {
    ConfigurationStore<ConfigurationStoreStructure> *store = nullptr;
    EepromKey key;
    const MemConfigBound<T> &bound;

    T data;

    operator T const &() const {
        return data;
    }
    void Init(ConfigurationStore<ConfigurationStoreStructure> *configuration_store);

    void Set(T new_data);
    T Get() {
        return data;
    }

    constexpr MemConfigItem(EepromKey key, const MemConfigBound<T> &bound)
        : key(key)
        , bound(bound)
        , data(bound.def) {}
};

template <class T, size_t SIZE>
struct MemConfigItem<std::array<T, SIZE>> {
    ConfigurationStore<ConfigurationStoreStructure> *store = nullptr;
    EepromKey key;
    const MemConfigBound<T> &bound;

    std::array<T, SIZE> data;

    operator std::array<T, SIZE> const &() const {
        return data;
    }
    void Init(ConfigurationStore<ConfigurationStoreStructure> *configuration_store);

    void Set(std::array<T, SIZE> new_data);
    const std::array<T, SIZE> &Get() {
        return data;
    }

    constexpr MemConfigItem(EepromKey key, const MemConfigBound<T> &bound)
        : key(key)
        , bound(bound) {
        data.fill(bound.def);
    }
};
