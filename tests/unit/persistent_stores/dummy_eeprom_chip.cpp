
#include "st25dv64k.h"
#include "dummy_eeprom_chip.h"
#include <cstring>
#include <catch2/catch.hpp>
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
uint8_t DummyEepromChip::read_byte(uint16_t address) {
    return get(address);
}
void DummyEepromChip::read_bytes(uint16_t address, std::span<uint8_t> buffer) {
    get(address, buffer.data(), buffer.size());
}
void DummyEepromChip::write_byte(uint16_t address, uint8_t data) {
    set(address, data);
}
void DummyEepromChip::write_bytes(uint16_t address, std::span<const uint8_t> data) {
    set(address, data.data(), data.size());
}

void DummyEepromChip::erase_area(uint16_t, uint16_t) {
    data.fill(0xff);
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    eeprom_chip.get(address, static_cast<uint8_t *>(pdata), size);
}

void st25dv64k_user_write_bytes(uint16_t address, void const *pdata, uint16_t size) {
    eeprom_chip.set(address, static_cast<const uint8_t *>(pdata), size);
}
uint8_t st25dv64k_user_read(uint16_t address) {
    return eeprom_chip.get(address);
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    eeprom_chip.set(address, data);
}
