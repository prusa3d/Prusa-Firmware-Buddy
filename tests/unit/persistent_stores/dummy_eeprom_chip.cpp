#include "dummy_eeprom_chip.h"

#include <cstring>
#include <catch2/catch_test_macros.hpp>

DummyEepromChip eeprom_chip;

void DummyEepromChip::get(uint16_t address, uint8_t *pdata, uint16_t len) {
    if (address < data.size()) {
        REQUIRE(address < data.size());
    }
    if (address + len > data.size()) {
        REQUIRE(address + len <= data.size());
    }
    std::memcpy(pdata, data.data() + address, len);
}
void DummyEepromChip::set(uint16_t address, const uint8_t *pdata, uint16_t len) {
    REQUIRE(address < data.size());
    REQUIRE(address + len <= data.size());
    std::memcpy(data.data() + address, pdata, len);
}
void DummyEepromChip::check_data(uint16_t address, std::vector<uint8_t> data_to_check) {
    check_data(address, data.data(), data.size());
}
void DummyEepromChip::set(uint16_t address, const uint8_t byte) {
    REQUIRE(address < data.size());
    data[address] = byte;
}
uint16_t DummyEepromChip::set(uint16_t address, Bytes bytes) {
    REQUIRE(address + bytes.size() <= data.size());
    std::memcpy(data.data() + address, bytes.data(), bytes.size());
    return address + bytes.size();
}
uint8_t DummyEepromChip::get(uint16_t address) {
    REQUIRE(address < data.size());
    return data[address];
}
void DummyEepromChip::clear() {
    data.fill(0xff);
}
bool DummyEepromChip::is_clear() {
    for (size_t i = 0; i < data.size(); i++) {
        INFO("Current pos is " << i);
        REQUIRE(data[i] == 0xff);
        if (data[i] != 0xff) {
            return false;
        }
    }
    return true;
}
void DummyEepromChip::check_data(uint16_t address, const uint8_t *data_to_check, std::size_t len) {
    REQUIRE(address < data.size());
    REQUIRE(address + len <= data.size());
    REQUIRE(0 == memcmp(data_to_check, data.data() + address, len));
}
std::span<uint8_t> DummyEepromChip::get(uint16_t address, std::size_t size) {
    REQUIRE(address < data.size());
    REQUIRE(address + size <= data.size());
    return { data.data() + address, size };
}
size_t DummyEepromChip::read_bytes(size_t address, WritableBytes buffer) {
    get(address, reinterpret_cast<uint8_t *>(buffer.data()), buffer.size());
    return buffer.size();
}
size_t DummyEepromChip::write_bytes(size_t address, Bytes data) {
    set(address, reinterpret_cast<const uint8_t *>(data.data()), data.size());
    return data.size();
}
