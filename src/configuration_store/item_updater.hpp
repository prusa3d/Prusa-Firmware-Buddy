#pragma once
#include <vector>
#include <stdint.h>
#include "configuration_store_structure.hpp"

struct ItemUpdater {
    ConfigurationStore<> &store;
    ItemUpdater(ConfigurationStore<> &store)
        : store(store) {}
    void operator()(uint32_t crc, const std::vector<uint8_t> &data);
};
