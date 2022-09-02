#include <catch2/catch.hpp>
#include "crc32.h"
#include "eeprom_acces.h"
#include "configuration_store.hpp"
#include "dummy_eeprom_chip.h"

static constexpr const char *key = "data";
static constexpr size_t eeprom_access_size = 10;
static constexpr size_t header_size = 3;
TEST_CASE("Basic data serialization") {
    auto data = EepromAccess<eeprom_access_size>::serialize_data<int>(key, 32);
    auto unpacked = EepromAccess<eeprom_access_size>::deserialize_data<int>(data);
    REQUIRE(unpacked.data == 32);
    REQUIRE(unpacked.key == crc32_calc(reinterpret_cast<const uint8_t *>(key), strlen(key)));
}

TEST_CASE("Test item storing and loading") {
    std::vector<uint8_t> data { 0x11, 0x22, 0x33, 0x44 };
    EepromAccess<eeprom_access_size> eeprom;
    eeprom.store_item(data);
    // just checking if data are stored in place we want, not checking if they have right header and crc
    eeprom_chip.check_data(EepromAccess<eeprom_access_size>::START_OFFSET + header_size, data);
    // Check that next offset was calculated
    REQUIRE(eeprom.first_free_space == header_size + data.size() + 4 + EepromAccess<eeprom_access_size>::START_OFFSET);
    // check that eeprom has set stop
    REQUIRE(eeprom_chip.get(eeprom.first_free_space) == EepromAccess<eeprom_access_size>::LAST_ITEM_STOP);

    SECTION("Storing and loading succesful") {
        auto read_data_opt = eeprom.load_item(EepromAccess<eeprom_access_size>::START_OFFSET);
        REQUIRE(read_data_opt.has_value());
        auto read_data = read_data_opt.value();
        REQUIRE(read_data == data);
    }

    SECTION("Corrupted data - wrong ext 8 id ") {
        eeprom_chip.set(EepromAccess<eeprom_access_size>::START_OFFSET, 0xff);
        auto item = eeprom.load_item(EepromAccess<eeprom_access_size>::START_OFFSET);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Corrupted data -  wrong custom type id") {
        eeprom_chip.set(EepromAccess<eeprom_access_size>::START_OFFSET + 2, 0xff);
        auto item = eeprom.load_item(EepromAccess<eeprom_access_size>::START_OFFSET);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Corrupted data -  wrong size") {
        eeprom_chip.set(EepromAccess<eeprom_access_size>::START_OFFSET + 1, 14);
        auto item = eeprom.load_item(EepromAccess<eeprom_access_size>::START_OFFSET);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Corrupted data - bad crc") {
        eeprom_chip.set(EepromAccess<eeprom_access_size>::START_OFFSET + 8, 0xff);
        auto item = eeprom.load_item(EepromAccess<eeprom_access_size>::START_OFFSET);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Loading failed") {
        EepromAccess<eeprom_access_size> eeprom;
        // there should be only ones on this address;
        auto item = eeprom.load_item(0);
        REQUIRE(item.has_value() == false);
    }
}

TEST_CASE("Multiple saves and loads") {
    std::vector<std::vector<uint8_t>> data = { { 0x11, 0x22, 0x33, 0x44 }, { 0x11, 0x44 }, { 0x11, 0x33, 0x44 }, { 0x11 }, { 0x11, 0x11, 0x22, 0x33, 0x44 } };
    size_t read_address = EepromAccess<eeprom_access_size>::START_OFFSET;
    EepromAccess<eeprom_access_size> eeprom;
    for (const auto &elem : data) {
        eeprom.store_item(elem);
    }
    for (const auto &elem : data) {
        auto read_data = eeprom.load_item(read_address);
        REQUIRE(read_data.has_value());
        REQUIRE(read_data.value() == elem);
        // move address by size of data, header and crc
        read_address += read_data->size() + header_size + 4;
    }
}

TEST_CASE("Save and load data") {
    std::array<std::pair<const char *, uint32_t>, 5> data_to_store = { { { "id1", 5532 }, { "id2", 5533 }, { "id3", 5534 }, { "id4", 5536 }, { "id5", 5538 } } };

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<uint32_t> distribution(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max());
    EepromAccess<eeprom_access_size> eeprom;

    for (auto &elem : data_to_store) {
        elem.second = distribution(g);
        eeprom.Set(elem.first, elem.second);
    }

    std::shuffle(data_to_store.begin(), data_to_store.end(), g);
    eeprom.Init();

    for (const auto &elem : data_to_store) {
        auto data = eeprom.Get<decltype(elem.second)>(elem.first);
        REQUIRE(data.has_value());
        REQUIRE(data.value() == elem.second);
    }
    {
        auto data = eeprom.Get<uint32_t>("id");
        REQUIRE(!data.has_value());
    }

    // save the ids with different value
    for (auto &elem : data_to_store) {
        elem.second = distribution(g);
        eeprom.Set(elem.first, elem.second);
    }
    std::shuffle(data_to_store.begin(), data_to_store.end(), g);
    eeprom.Init();

    for (const auto &elem : data_to_store) {
        auto data = eeprom.Get<decltype(elem.second)>(elem.first);
        REQUIRE(data.has_value());
        REQUIRE(data.value() == elem.second);
    }
    auto data = eeprom.Get<uint32_t>("id");
    REQUIRE(!data.has_value());
}

TEST_CASE("Cleanup") {
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<uint32_t> distribution(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max());
    EepromAccess<eeprom_access_size> eeprom;
    // size of crc + header + data
    uint32_t data = 0;
    while (eeprom.first_free_space < (EepromAccess<eeprom_access_size>::LAST_ITEM_STOP - 2 * sizeof(uint32_t) - header_size)) {
        data = distribution(g);
        eeprom.Set("id", data);
        eeprom.Init();
        auto read_data = eeprom.Get<uint32_t>("id");
        REQUIRE(read_data.has_value());
        REQUIRE(read_data.value() == data);
    }
    uint16_t last_free_space = eeprom.first_free_space;
    data = distribution(g);
    eeprom.Set("id", data);

    eeprom.Init();
    auto read_data = eeprom.Get<uint32_t>("id");
    REQUIRE(read_data.has_value());
    REQUIRE(read_data.value() == data);
    REQUIRE(last_free_space < eeprom.first_free_space);
}

TEST_CASE("Eeprom") {
    EepromAccess<ConfigurationStore<>::NUM_OF_ITEMS> eeprom_access;
    auto eeprom = new ConfigurationStore(eeprom_access);
    eeprom->Init();
    eeprom->version.Set(EepromVersion::v4);
    delete eeprom;

    eeprom = new ConfigurationStore(eeprom_access);
    eeprom->Init();
    REQUIRE(eeprom->version.Get() == EepromVersion::v4);
}
