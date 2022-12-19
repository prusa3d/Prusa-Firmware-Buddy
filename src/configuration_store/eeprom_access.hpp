#pragma once
#include <vector>
#include <stdint.h>
#include "crc32.h"
#include <stdlib.h>
#include <string.h>
#include "msgpack.h"
#include <optional>
#include "st25dv64k.h"
#include <algorithm>
#include "item_updater.hpp"
#include "bsod.h"
#include "freertos_mutex.hpp"
#include <mutex>
#include "hash_table.hpp"
LOG_COMPONENT_REF(EEPROM_ACCESS);
namespace configuration_store { // helper structs

struct Key {
    uint32_t key;
    template <class Packer>
    void pack(Packer &pack) {
        pack(key);
    }
};

struct InvalidatedItem {
    uint32_t key;
    std::nullptr_t nil;

    template <class Packer>
    void pack(Packer &pack) {
        pack(key, nil);
    }
};

/**
 * This struct is serialized to eeprom
 * @tparam T type of stored data
 */
template <class T>
struct DataItem {
    /** @brief crc32 of item name  */
    uint32_t key;
    T data;

    DataItem(uint32_t key, T data)
        : key(key)
        , data(data) {}
    DataItem() = default;

    template <class Packer>
    void pack(Packer &pack) {
        pack(key, data);
    }
};
/**
 * @brief This class enables access to eeprom as journaling filesystem
 *
 * It splits the eeprom in two banks and writes to one util it is full and then switches to the other. When it is switching it copies the newest value for each key to the start of the new bank
 * The old bank is left valid as backup if the current bank gets corrupted.
 *
 * The filesystem supports only Set operation, the data are read only on initialization because Get operation without caching would have O(n) complexity// stores item to eeprom and calculates the crc of the data, will do cleanup if there is not enough space left in the bank to store the item
 *
 * One item is stored as:
 * +--------+========+--------+
 * |XXXXXXXX|  data  |  crc   |
 * +--------+========+--------+
 * where XXXXXXXX is a 8-bit unsigned integer which represents the length of the data including crc
 * it appends 0xff after the item to signal that it is the last item in the eeprom
 *
 * data part is DataItem serialized with msgpack.
 */
class EepromAccess {
    friend ItemUpdater;
#ifdef EEPROM_UNITTEST
public:
#endif

    static constexpr size_t NUM_OF_ITEMS = ConfigurationStoreStructure::NUM_OF_ITEMS;

    static constexpr uint16_t BANK_SELECTOR_ADDRESS = 0x500;

    static constexpr uint16_t START_OFFSET
        = 0x500 + 700; // offset to save data after current eeprom data
    static constexpr uint16_t EEPROM_SIZE = 8096;
    static constexpr uint16_t BANK_SIZE = (EEPROM_SIZE - START_OFFSET) / 2;
    static constexpr uint16_t MIN_FREE_SPACE = 512;

    static constexpr uint8_t LAST_ITEM_STOP = 0xfe;
    static constexpr size_t HEADER_SIZE = 1;
    static constexpr size_t CRC_SIZE = 4;
    static constexpr size_t NIL_POSITION = 5;

    uint16_t first_free_space = START_OFFSET;

    template <class T>
    std::vector<uint8_t> static serialize_data(const char *key, T data);
    template <class T>
    DataItem<T> static deserialize_data(std::vector<uint8_t> data);

    void invalidate_items(const HashMap<NUM_OF_ITEMS> &items_to_invalidate);

    // stores item to eeprom and calculates the crc of the data, will do cleanup if there is not enough space left in the bank to store the item
    // +--------+========+--------+
    // |XXXXXXXX|  data  |  crc   |
    // +--------+========+--------+
    // where XXXXXXXX is a 8-bit unsigned integer which represents the length of the data including crc
    // it appends 0xff after the item to signal that it is the last item in the eeprom
    // returns false if the data couldnt fit to bank and switches to second bank
    bool store_item(const std::vector<uint8_t> &data);

    // load item from eeprom, if item is invalid it returns nullopt
    std::optional<std::vector<uint8_t>> load_item(uint16_t address);

    void switch_bank() {
        if (bank_selector == 1) {
            bank_selector = 0;
        } else {
            bank_selector = 1;
        }
    }
    constexpr uint16_t get_start_offset() {
        return bank_selector == 1 ? START_OFFSET + BANK_SIZE : START_OFFSET;
    }
    /**
     * init items starting on address
     * @param updater
     * @param address from which to start the initialization,
     * @param address will be pointing to end of the bank
     * @return true if data is valid false otherwise
     */
    bool init_from(ItemUpdater &updater, uint16_t &address);

    /**
     * checks if write will fit inside current bank
     * @param address
     * @param size
     * @return true if it will fit false otherwise
     */
    bool check_size(uint8_t size);

    /**
     * @brief Removes duplicates of data and saves unique keys to the beginning of eeprom
     */
    void cleanup();
    uint8_t bank_selector;
    FreeRTOS_Mutex mutex;

public:
    /**
     * @brief Initializes index with valid items in eeprom
     *
     * Iterates through eeprom, checks validity of the data and adds them to index
     */
    void init(ItemUpdater &updater);

    /**
     * @brief Saves data item after last item in eeprom
     *
     * This function saves new item by appending it after the last item in eeprom. If we cant fit the new item in eeprom we will copy newest items to the beginning of eeprom
     * Each item consists od crc32 of the key and the data. This is serialized with msgpack and stored by store_item function.
     * @tparam T
     * @param key
     * @param data
     */
    template <class T>
    bool set(const char *key, const T &data);

    /**
     * @brief Invalidates the values stored in eeprom in both banks
     *
     * Writes 0xff to each address in eeprom
     * Restart or reinitialization of eeprom is needed after this function
     */
    void reset();
    EepromAccess() = default;
    EepromAccess(EepromAccess &&other);
    static EepromAccess &instance();
};
template <class T>
bool EepromAccess::set(const char *key, const T &data) {
    auto serialized = serialize_data(key, data);

    std::unique_lock lock(mutex);
    return store_item(serialized);
}
struct IndexComparator {
    bool operator()(const std::pair<uint32_t, uint16_t> &a, const std::pair<uint32_t, uint16_t> &b) {
        return a.first < b.first;
    }
};

template <class T>
std::vector<uint8_t> EepromAccess::serialize_data(const char *key, T data) {
    std::size_t len = strlen(key);
    uint32_t name_hash = crc32_sw(reinterpret_cast<const uint8_t *>(key), len, 0);
    log_debug(EEPROM_ACCESS, "Saving item %s with id: %d", key, name_hash);
    DataItem<T> item = DataItem(name_hash, data);
    auto data_packed = (msgpack::pack(item));
    return data_packed;
}

template <class T>
DataItem<T> EepromAccess::deserialize_data(std::vector<uint8_t> data) {
    DataItem<T> unpacked = msgpack::unpack<DataItem<T>>(data);
    return unpacked;
}

}
