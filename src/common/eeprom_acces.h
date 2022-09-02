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
#ifndef EEPROM_UNITTEST
    #include "bsod.h"
#endif

// helper structs
struct Key {
    uint32_t key;
    template <class Packer>
    void pack(Packer &pack) {
        pack(key);
    }
};

template <class T>
struct DataItem {
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

template <size_t NUM_OF_ITEMS>
class EepromAccess {
#ifdef EEPROM_UNITTEST
public:
#endif
    static constexpr uint16_t START_OFFSET = 0x500 + 700; // offset to save data after current eeprom data
    static constexpr uint8_t CUSTOM_ITEM_ID = 0x7f;
    static constexpr uint8_t MSGPACK_EXT8 = 0xc7;
    static constexpr uint8_t LAST_ITEM_STOP = 0xfe;
    static constexpr uint16_t EEPROM_SIZE = 8096;
    static constexpr size_t HEADER_SIZE = 3;
    static constexpr size_t CRC_SIZE = 4;

    size_t num_of_indexes = 0;
    std::array<std::pair<uint32_t, uint16_t>, NUM_OF_ITEMS> index;

    uint16_t first_free_space = START_OFFSET;

    template <class T>
    std::vector<uint8_t> static serialize_data(const char *key, T data);
    template <class T>
    DataItem<T> static deserialize_data(std::vector<uint8_t> data);

    // find address for hash in index
    std::optional<uint16_t> get_address(uint32_t hash);

    // stores item to eeprom and calculates the crc of the data
    // creates header of the data
    // the header  element ext 8 from msgpack spec and the value for type is EepromAccess::CUSTOM_ITEM_ID
    // +--------+--------+--------+========+--------+
    // |  0xc7  |XXXXXXXX|  type  |  data  |  crc   |
    // +--------+--------+--------+========+--------+
    // where XXXXXXXX is a 8-bit unsigned integer which represents the length of the data including crc
    // it appends 0xff after the item to signal that it is the last item in the eeprom
    void store_item(const std::vector<uint8_t> &data);

    // load item from eeprom, if item is invalid it returns nullopt
    std::optional<std::vector<uint8_t>> load_item(uint16_t address);
    bool initialized = false;

public:
    /**
     * @brief Initializes index with valid items in eeprom
     *
     * Iterates through eeprom, checks validity of the data and adds them to index
     */
    void Init();

    /**
     * @brief Removes duplicates of data and saves unique keys to the beginning of eeprom
     */
    void Cleanup();

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
    void Set(const char *key, const T &data);

    /**
     * @brief loads item with key from eeprom if it is initialized
     *
     * Loads the item from eeprom by using index, which is created by calling Init(). The index is invalidated by calling set
     * Should be called only on startup, not during runtime
     *
     * @tparam T
     * @param key
     * @return nullopt if data isn't in eeprom, or are corrupted or the index is not initialized, else it returns the desired data
     */
    template <class T>
    std::optional<T> Get(const char *key);
};
template <size_t NUM_OF_ITEMS>
template <class T>
void EepromAccess<NUM_OF_ITEMS>::Set(const char *key, const T &data) {
    initialized = false;
    auto serialized = serialize_data(key, data);
    store_item(serialized);
}
struct IndexComparator {
    bool operator()(const std::pair<uint32_t, uint16_t> &a, const std::pair<uint32_t, uint16_t> &b) {
        return a.first < b.first;
    }
};

template <size_t NUM_OF_ITEMS>
template <class T>
std::optional<T> EepromAccess<NUM_OF_ITEMS>::Get(const char *key) {
    if (!initialized) {
        // TODO maybe call general_error,this is untended usage and is fault of the programmer?
        return std::nullopt;
    }

    std::size_t len = strlen(key);
    uint32_t name_hash = crc32_calc(reinterpret_cast<const uint8_t *>(key), len);

    auto address = get_address(name_hash);
    if (!address.has_value()) {
        return std::nullopt;
    }

    auto data = load_item(address.value());

    if (data.has_value()) {
        return deserialize_data<T>(data.value()).data;
    }
    return std::nullopt;
}

template <size_t NUM_OF_ITEMS>
template <class T>
std::vector<uint8_t> EepromAccess<NUM_OF_ITEMS>::serialize_data(const char *key, T data) {
    std::size_t len = strlen(key);
    uint32_t name_hash = crc32_calc(reinterpret_cast<const uint8_t *>(key), len);
    DataItem<T> item = DataItem(name_hash, data);
    auto data_packed = (msgpack::pack(item));
    return data_packed;
}

template <size_t NUM_OF_ITEMS>
template <class T>
DataItem<T> EepromAccess<NUM_OF_ITEMS>::deserialize_data(std::vector<uint8_t> data) {
    DataItem<T> unpacked = msgpack::unpack<DataItem<T>>(data);
    return unpacked;
}
template <size_t NUM_OF_ITEMS>
void EepromAccess<NUM_OF_ITEMS>::store_item(const std::vector<uint8_t> &data) {
    // we have only one byte to store length of the data
    if (data.size() >= (std::numeric_limits<uint8_t>::max() - 1)) {
#ifdef EEPROM_UNITTEST
        throw std::out_of_range("address not in range of eeprom");
#else
        general_error("data too large", "eeprom");
#endif
    }
    // add 4 to size,because we will add crc at the end of the data
    std::array<uint8_t, 3> header { MSGPACK_EXT8, static_cast<unsigned char>(data.size() + 4), CUSTOM_ITEM_ID };

    uint32_t crc = crc32_calc(header.data(), header.size());
    crc = crc32_calc_ex(crc, data.data(), data.size());

    uint16_t size_of_item = header.size() + data.size() + sizeof(crc);

    if ((size_of_item + first_free_space) >= EEPROM_SIZE) {
        Cleanup();
    }

    // persist data in eeprom
    // write stop first, then crc,then data and last write the header, because we want to rewrite stop as the last thing in eeprom
    // we want to write the stop only if we are writing to new
    st25dv64k_user_write(size_of_item + first_free_space, LAST_ITEM_STOP);
    st25dv64k_user_write_bytes(first_free_space + size_of_item - sizeof(crc), &crc, sizeof(crc));
    st25dv64k_user_write_bytes(first_free_space + header.size(), data.data(), data.size());
    st25dv64k_user_write_bytes(first_free_space, header.data(), header.size());
    first_free_space += size_of_item;
}

template <size_t NUM_OF_ITEMS>
std::optional<std::vector<uint8_t>> EepromAccess<NUM_OF_ITEMS>::load_item(uint16_t address) {
    std::array<uint8_t, 3> header { 0, 0, 0 };

    if ((address + sizeof(header)) > EEPROM_SIZE) {
        return std::nullopt;
    }
    st25dv64k_user_read_bytes(address, header.data(), header.size());

    if (header[0] != MSGPACK_EXT8 || header[2] != CUSTOM_ITEM_ID) {
        return std::nullopt;
    }
    std::vector<uint8_t> data;
    // add 4 to load also the crc
    data.resize(header[1]);

    if ((address + sizeof(header) + data.size()) > EEPROM_SIZE) {
        return std::nullopt;
    }

    st25dv64k_user_read_bytes(address + header.size(), data.data(), data.size());

    uint32_t crc = crc32_calc(header.data(), header.size());
    crc = crc32_calc_ex(crc, data.data(), data.size() - sizeof(crc));

    uint32_t crc_read = *(uint32_t *)((data.data() + data.size() - sizeof(crc)));

    if (crc != crc_read) {
        return std::nullopt;
    }
    // remove crc from the data
    data.resize(data.size() - sizeof(crc));
    return data;
}

template <size_t NUM_OF_ITEMS>
void EepromAccess<NUM_OF_ITEMS>::Cleanup() {
    // get current position of newest elements;
    Init();
    first_free_space = START_OFFSET;
    // load data from old position and store them
    for (size_t i = 0; i < num_of_indexes; i++) {
        auto data = load_item(index[i].second);
        if (data.has_value()) {
            store_item(data.value());
        }
    }
}

template <size_t NUM_OF_ITEMS>
void EepromAccess<NUM_OF_ITEMS>::Init() {

    // fill it with invalid values
    index.fill({ std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint16_t>::max() });

    uint16_t address = START_OFFSET;
    num_of_indexes = 0;
    for (auto item = load_item(address); item.has_value(); item = load_item(address)) {
        bool found = false;
        auto data = item.value();
        uint32_t key = msgpack::unpack<Key>(data).key;
        // TODO this is not effective, refactor it to make it more effective
        for (size_t i = 0; i < num_of_indexes; i++) {
            auto &[key_stored, address_stored] = index[i];
            if (key_stored == key) {
                address_stored = address;
                found = true;
                break;
            }
        }

        // if key not found push back and sort
        if (!found) {
            index[num_of_indexes++] = std::pair { key, address };
            std::push_heap(index.begin(), index.begin() + num_of_indexes);
        }

        address += item.value().size() + HEADER_SIZE + CRC_SIZE;
    }
    std::sort_heap(index.begin(), index.begin() + num_of_indexes);
    initialized = true;
}

template <size_t NUM_OF_ITEMS>
std::optional<uint16_t> EepromAccess<NUM_OF_ITEMS>::get_address(uint32_t hash) {
    auto it = std::lower_bound(index.begin(), index.begin() + num_of_indexes, std::pair<uint32_t, uint16_t> { hash, 0 }, [](const std::pair<uint32_t, uint16_t> &a, const std::pair<uint32_t, uint16_t> &b) {
        return a.first < b.first;
    });
    if (it != (index.begin() + num_of_indexes) && it->first == hash) {
        return it->second;
    }
    return std::nullopt;
}
