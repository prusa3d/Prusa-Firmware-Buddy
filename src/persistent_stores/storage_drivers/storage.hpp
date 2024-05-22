#pragma once
#include <span>
#include <stdint.h>

namespace configuration_store {

class Storage {

public:
    virtual uint8_t read_byte(uint16_t address) = 0;
    virtual void read_bytes(uint16_t address, std::span<uint8_t> buffer) = 0;
    virtual void write_byte(uint16_t address, uint8_t data) = 0;
    virtual void write_bytes(uint16_t address, std::span<const uint8_t> data) = 0;
    virtual void erase_area(uint16_t start_address, uint16_t end_address) = 0;
    Storage() = default;
    virtual ~Storage() = default;
    Storage(const Storage &other) = delete;
    Storage(Storage &&other) = delete;
    Storage &operator=(const Storage &other) = delete;
    Storage &operator=(Storage &&other) = delete;
};
} // namespace configuration_store
