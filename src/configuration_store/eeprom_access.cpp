#include "eeprom_access.hpp"
#include "hash_table.hpp"
#include "timing.h"
#include "HAL/HAL.h"
LOG_COMPONENT_DEF(EEPROM_ACCESS, LOG_SEVERITY_DEBUG);
using namespace configuration_store;
bool EepromAccess::store_item(const std::vector<uint8_t> &data) {
    log_debug(EEPROM_ACCESS, "Store item with size %d", data.size());
    // we have only one byte to store length of the data and 0xff is invalid length value
    if (data.size() >= (std::numeric_limits<uint8_t>::max() - 1)) {
        fatal_error("data too large", "eeprom");
    }
    // add 4 to size,because we will add crc at the end of the data
    uint8_t len = data.size() + 4;

    uint32_t crc = crc32_sw(&len, sizeof(len), 0);

    crc = crc32_sw(data.data(), data.size(), crc);

    uint16_t size_of_item = sizeof(len) + data.size() + sizeof(crc);

    // check if the item will fit inside current bank
    if (check_size(size_of_item)) {
        cleanup();
    }

    // persist data in eeprom
    // write stop first, then crc,then data and last write the header, because we want to rewrite stop as the last thing in eeprom
    // we want to write the stop only if we are writing to new
    st25dv64k_user_write(size_of_item + first_free_space, LAST_ITEM_STOP);
    st25dv64k_user_write_bytes(first_free_space + size_of_item - sizeof(crc), &crc, sizeof(crc));
    st25dv64k_user_write_bytes(first_free_space + sizeof(len), data.data(), data.size());
    st25dv64k_user_write(first_free_space, len);
    first_free_space += size_of_item;
    return true;
}

std::optional<std::vector<uint8_t>> EepromAccess::load_item(uint16_t address) {
    uint8_t len = 0;

    if ((address + sizeof(len)) > EEPROM_SIZE) {
        return std::nullopt;
    }
    len = st25dv64k_user_read(address);

    // we cant have item smaller than length of crc and 0xff is default value of eeprom cell
    if (len <= 4 || len == 0xff) {
        return std::nullopt;
    }

    std::vector<uint8_t> data;
    data.resize(len);

    if ((address + sizeof(len) + len) > EEPROM_SIZE) {
        return std::nullopt;
    }

    st25dv64k_user_read_bytes(address + sizeof(len), data.data(), data.size());

    uint32_t crc = crc32_sw(&len, sizeof(len), 0);
    crc = crc32_sw(data.data(), data.size() - sizeof(crc), crc);

    uint32_t crc_read = *(uint32_t *)((data.data() + data.size() - sizeof(crc)));

    if (crc != crc_read) {
        return std::nullopt;
    }
    // remove crc from the data
    data.resize(data.size() - sizeof(crc));
    return data;
}

void EepromAccess::cleanup() {
    log_info(EEPROM_ACCESS, "Cleanup started");
    uint32_t start = ticks_ms();
    HashMap<NUM_OF_ITEMS> map;
    uint16_t address = get_start_offset();
    for (auto item = load_item(address); item.has_value(); item = load_item(address)) {
        auto data = item.value();
        Key key = msgpack::unpack<Key>(data);
        if (data[NIL_POSITION] == msgpack::nil) {
            // remove invalidated items
            map.remove(key.key);
        }
        map.Set(key.key, address);
        address += HEADER_SIZE + CRC_SIZE + data.size();
    }

    // reset offset
    switch_bank();
    first_free_space = get_start_offset();

    for (auto &[key, read_address] : map) {
        auto data = load_item(read_address);
        // all data should have value, if not something corrupted eeprom between reads
        if (data.has_value()) {
            store_item(data.value());
        } else {
            fatal_error("Data in eeprom corrupted", "eeprom");
        }
    }

    // update bank selector to current bank
    st25dv64k_user_write(BANK_SELECTOR_ADDRESS, bank_selector);

    log_info(EEPROM_ACCESS, "Cleanup took %u ms", ticks_diff(ticks_ms(), start));
}

void EepromAccess::init(ItemUpdater &updater) {
    std::unique_lock lock(mutex);
    uint32_t start = ticks_ms();

    bank_selector = st25dv64k_user_read(BANK_SELECTOR_ADDRESS);
    if (bank_selector > 1) {
        bank_selector = 0;
    }

    uint16_t address = get_start_offset();
    bool res = init_from(updater, address);
    if (!res) {
        log_error(EEPROM_ACCESS, "Initialization from bank: %d failed", bank_selector);
        // we have not iterated to the last item, this could mean that the eeprom is corrupted or not initialized
        switch_bank();
        address = get_start_offset();
        if (!init_from(updater, address)) {
            log_error(EEPROM_ACCESS, "Initialization from bank: %d failed,selecting bank 1", bank_selector);
            // both banks not valid, init on the start of the second bank because in the first one could be the old eeprom
            bank_selector = 1;
            address = get_start_offset();
        }
        // update bank selector to current bank
        st25dv64k_user_write(BANK_SELECTOR_ADDRESS, bank_selector);
    }

    log_debug(EEPROM_ACCESS, "Eeprom initialized");

    first_free_space = address;
    uint16_t free_space = BANK_SIZE - (address - get_start_offset());
    if (free_space < MIN_FREE_SPACE) {
        cleanup();
    }
    log_info(EEPROM_ACCESS, "Eeprom init took %u ms", ticks_diff(ticks_ms(), start));
}
EepromAccess &EepromAccess::instance() {
    static EepromAccess eeprom;
    return eeprom;
}
enum class ItemState : uint16_t {
    Valid,
    Invalid,
    Invalidated,
};

void EepromAccess::invalidate_items(const HashMap<NUM_OF_ITEMS> &items_to_invalidate) {
    for (auto &[key, item_validity] : items_to_invalidate) {
        if (static_cast<ItemState>(item_validity) == ItemState::Invalid) {
            // we want to invalidate invalid items
            InvalidatedItem item { key };
            store_item(msgpack::pack(item));
        }
    }
}
bool EepromAccess::init_from(ItemUpdater &updater, uint16_t &address) {
    HashMap<NUM_OF_ITEMS> invalid_items;
    size_t num_of_items = 0;
    for (auto item = load_item(address); item.has_value(); item = load_item(address)) {
        auto data = item.value();
        Key key = msgpack::unpack<Key>(data);
        // TODO create some way to tell updater that this item was already invalidated.
        if (data[NIL_POSITION] == msgpack::nil) {
            // already set to invalid in eeprom, we dont want to write another invalid item
            log_debug(EEPROM_ACCESS, "Found invalidated item with id: %d,", key.key);
            invalid_items.Set(key.key, static_cast<uint16_t>(ItemState::Invalidated));
            // tell updater that this item was invalidated and that means it was also migrated
            updater.updated(key.key);
        } else {
            if (!updater(key.key, data)) {
                // unknown id
                log_debug(EEPROM_ACCESS, "Found unknown item with id: %d,", key.key);
                invalid_items.Set(key.key, static_cast<uint16_t>(ItemState::Invalid));
            }
        }

        address += HEADER_SIZE + CRC_SIZE + data.size();
        num_of_items++;
    }
    log_info(EEPROM_ACCESS, "Loaded %d items", num_of_items);
    uint8_t ending_flag = st25dv64k_user_read(address);
    log_info(EEPROM_ACCESS, "loaded ending flag %u", ending_flag);
    first_free_space = address;
    if (ending_flag == LAST_ITEM_STOP) {
        invalidate_items(invalid_items);
    }
    return ending_flag == LAST_ITEM_STOP;
}

bool EepromAccess::check_size(uint8_t size) {
    uint16_t free_space = BANK_SIZE - (first_free_space - get_start_offset());
    log_debug(EEPROM_ACCESS, "Free space lef %d", free_space);
    return free_space < size;
}

void EepromAccess::reset() {
    std::unique_lock lock(mutex);
    uint16_t a;
    uint32_t data = 0xffffffff;
    for (a = 0x0000; a < 0x2000; a += 4) {
        st25dv64k_user_write_bytes(a, &data, 4);
        if ((a % 0x200) == 0)   // clear entire eeprom take ~4s
            watchdog_refresh(); // we will reset watchdog every ~1s for sure
    }
    bank_selector = 0;
    st25dv64k_user_write(BANK_SELECTOR_ADDRESS, bank_selector);
    first_free_space = START_OFFSET;
}
EepromAccess::EepromAccess(EepromAccess &&other)
    : first_free_space(other.first_free_space)
    , bank_selector(other.bank_selector)
    , mutex(std::move(mutex)) {}
