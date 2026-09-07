#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <storage_drivers/storage.hpp>
#include <utils/byte_utils.hpp>

class DummyEepromChip : public configuration_store::Storage {
    std::array<std::byte, 8096> memory;
    std::optional<std::size_t> write_limit = std::nullopt;

public:
    DummyEepromChip() {
        clear();
    }

    std::byte get(uint16_t address);
    Bytes get(uint16_t address, std::size_t size);
    void set(uint16_t address, std::byte byte);
    uint16_t set(uint16_t address, Bytes bytes);
    void clear();
    void set_write_limit(std::optional<std::size_t> limit);
    size_t read_bytes(size_t address, WritableBytes buffer) override;
    size_t write_bytes(size_t address, Bytes bytes) override;
    void flush() override {}
};
extern DummyEepromChip eeprom_chip;

inline configuration_store::Storage &get_eeprom_chip() {
    return eeprom_chip;
}
