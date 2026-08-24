#include "dummy_eeprom_chip.h"

#include <cstring>
#include <catch2/catch_test_macros.hpp>

DummyEepromChip eeprom_chip;

std::byte DummyEepromChip::get(uint16_t address) {
    REQUIRE(address < memory.size());
    return memory[address];
}

Bytes DummyEepromChip::get(uint16_t address, std::size_t size) {
    REQUIRE(address + size <= memory.size());
    return { memory.data() + address, size };
}

void DummyEepromChip::set(uint16_t address, std::byte byte) {
    REQUIRE(address < memory.size());
    memory[address] = byte;
}

uint16_t DummyEepromChip::set(uint16_t address, Bytes bytes) {
    REQUIRE(address + bytes.size() <= memory.size());
    std::memcpy(memory.data() + address, bytes.data(), bytes.size());
    const uint16_t next_free = address + bytes.size();
    return next_free;
}

void DummyEepromChip::clear() {
    memory.fill(std::byte { 0xff });
}

size_t DummyEepromChip::read_bytes(size_t address, WritableBytes buffer) {
    const auto bytes = get(address, buffer.size());
    std::memcpy(buffer.data(), bytes.data(), bytes.size());
    return buffer.size();
}
size_t DummyEepromChip::write_bytes(size_t address, Bytes bytes) {
    set(address, bytes);
    return bytes.size();
}
