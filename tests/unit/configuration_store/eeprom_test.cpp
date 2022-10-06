#include <catch2/catch.hpp>
#include "crc32.h"
#include "eeprom_access.hpp"
#include "configuration_store/configuration_store.hpp"
#include "dummy_eeprom_chip.h"
using namespace configuration_store;
static constexpr const char *key = "data";
static constexpr size_t header_size = 1;

TEST_CASE("Basic data serialization") {
    auto data = EepromAccess::serialize_data<int>(key, 32);
    auto unpacked = EepromAccess::deserialize_data<int>(data);
    REQUIRE(unpacked.data == 32);
    REQUIRE(unpacked.key == crc32_calc(reinterpret_cast<const uint8_t *>(key), strlen(key)));
}

TEST_CASE("Test item storing and loading") {
    std::vector<uint8_t> data { 0x11, 0x22, 0x33, 0x44 };
    eeprom_chip.clear();
    EepromAccess eeprom;
    ConfigurationStore<> store;
    auto updater = ItemUpdater(store);
    eeprom.init(updater);
    eeprom.store_item(data);
    // just checking if data are stored in place we want, not checking if they have right header and crc
    eeprom_chip.check_data(eeprom.get_start_offset() + header_size, data);
    // Check that next offset was calculated
    REQUIRE(eeprom.first_free_space == header_size + data.size() + 4 + EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE);
    // check that eeprom has set stop
    REQUIRE(eeprom_chip.get(eeprom.first_free_space) == EepromAccess::LAST_ITEM_STOP);

    SECTION("Storing and loading succesful") {
        auto read_data_opt = eeprom.load_item(EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE);
        REQUIRE(read_data_opt.has_value());
        auto read_data = read_data_opt.value();
        REQUIRE(read_data == data);
    }

    SECTION("Corrupted data -  wrong size") {
        eeprom_chip.set(EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE + 1, 14);
        auto item = eeprom.load_item(EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Corrupted data - bad crc") {
        eeprom_chip.set(EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE + 8, 0xff);
        auto item = eeprom.load_item(EepromAccess::START_OFFSET + EepromAccess::BANK_SIZE);
        REQUIRE(item.has_value() == false);
    }

    SECTION("Loading failed") {
        EepromAccess eeprom;
        // there should be only ones on this address;
        auto item = eeprom.load_item(0);
        REQUIRE(item.has_value() == false);
    }
}

TEST_CASE("Multiple saves and loads") {
    std::vector<std::vector<uint8_t>> data = { { 0x11, 0x22, 0x33, 0x44 }, { 0x11, 0x44 }, { 0x11, 0x33, 0x44 }, { 0x11 }, { 0x11, 0x11, 0x22, 0x33, 0x44 } };
    eeprom_chip.clear();
    EepromAccess eeprom;
    ConfigurationStore<> store;
    auto updater = ItemUpdater(store);
    eeprom.init(updater);
    size_t read_address = eeprom.get_start_offset();
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

TEST_CASE("Cleanup") {
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<int32_t> distribution(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();
    // size of crc + header + data
    int32_t data = 0;
    uint8_t bank = EepromAccess::instance().bank_selector;
    REQUIRE(bank == 1);
    while (EepromAccess::instance().bank_selector == bank) {
        data = distribution(g);
        uint16_t address = EepromAccess::instance().first_free_space;
        eeprom->SimpleType.set(data);
        REQUIRE(address < EepromAccess::instance().first_free_space);
        address = EepromAccess::instance().first_free_space;
        delete eeprom;
        eeprom = new ConfigurationStore();
        eeprom->init();
        // if we change bank in init the address will be different
        REQUIRE((bank != EepromAccess::instance().bank_selector || address == EepromAccess::instance().first_free_space));
        REQUIRE(eeprom->SimpleType.get() == data);
    }
    delete eeprom;
    eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(EepromAccess::instance().bank_selector == 0);

    REQUIRE(eeprom->SimpleType.get() == data);
}

TEST_CASE("Eeprom") {
    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();
    eeprom->SimpleType.set(10);
    REQUIRE(eeprom->SimpleType.get() == 10);
    eeprom->SimpleType.set(20);
    REQUIRE(eeprom->SimpleType.get() == 20);
    eeprom->SimpleType.set(10);
    REQUIRE(eeprom->SimpleType.get() == 10);
    delete eeprom;

    eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(eeprom->SimpleType.get() == 10);
}

TEST_CASE("Factory reset") {
    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(eeprom->SimpleType.get() == 0);

    auto struct_type = eeprom->struct_type.get();
    struct_type.SimpleType = 10;
    struct_type.array = { 10, 10 };
    struct_type.string_type = { "test" };
    eeprom->struct_type.set(struct_type);

    eeprom->SimpleType.set(10);
    REQUIRE(eeprom->SimpleType.get() == 10);
    eeprom->factory_reset();
    delete eeprom;
    eeprom = new ConfigurationStore();
    eeprom->init();
    struct_type = eeprom->struct_type.get();
    REQUIRE(eeprom->SimpleType.get() == 0);
    REQUIRE(struct_type.SimpleType == 0);
    REQUIRE(struct_type.array == std::array<int32_t, 10> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    REQUIRE(strcmp(struct_type.string_type.data(), "test string") == 0);
}
TEST_CASE("String") {
    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(strcmp(eeprom->string_type.get(), "test string") == 0);

    eeprom->string_type.set("test");
    REQUIRE(strcmp(eeprom->string_type.get(), "test") == 0);
    delete eeprom;
    eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(strcmp(eeprom->string_type.get(), "test") == 0);
}
TEST_CASE("Array") {
    eeprom_chip.clear();
    std::array<int32_t, 10> data {};
    data.fill(0);

    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(eeprom->array.get() == data);
    data.fill(10);
    eeprom->array.set(data);
    REQUIRE(eeprom->array.get() == data);
    delete eeprom;
    eeprom = new ConfigurationStore();
    eeprom->init();
    REQUIRE(eeprom->array.get() == data);
}
TEST_CASE("Struct") {
    eeprom_chip.clear();
    auto eeprom = new ConfigurationStore();
    eeprom->init();

    auto struct_type = eeprom->struct_type.get();
    struct_type.SimpleType = 10;
    struct_type.array = { 10, 10 };
    struct_type.string_type = { "test" };
    eeprom->struct_type.set(struct_type);

    delete eeprom;
    eeprom = new ConfigurationStore();
    eeprom->init();

    struct_type = eeprom->struct_type.get();
    REQUIRE(struct_type.SimpleType == 10);
    REQUIRE(struct_type.array == std::array<int32_t, 10> { 10, 10, 0, 0, 0, 0, 0, 0, 0, 0 });
    REQUIRE(strcmp(struct_type.string_type.data(), "test") == 0);
}
