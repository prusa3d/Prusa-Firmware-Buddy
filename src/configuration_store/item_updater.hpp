#pragma once
#include <vector>
#include <stdint.h>
#include "configuration_store_structure.hpp"
namespace configuration_store {
class ItemUpdater {
public:
    ItemUpdater(ConfigurationStore &store)
        : store(store) {}
    /**
     * @param crc
     * @param data
     * @return returns true if the item is valid or unsupported but already invalidated, true if item is unsupported
     */
    bool operator()(uint32_t crc, const std::vector<uint8_t> &data);
    void updated(uint32_t crc);
    void migrate();

private:
    DeprecatedItems deprecated_items;
    ConfigurationStore &store;
};

};
