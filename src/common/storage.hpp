#pragma once
#include <span>
#include <stdint.h>

class Storage {

public:
    virtual uint8_t read_byte(uint16_t address) = 0;
    virtual void read_bytes(uint16_t address, std::span<uint8_t> buffer) = 0;
    virtual void write_byte(uint16_t address, uint8_t data) = 0;
    virtual void write_bytes(uint16_t address, std::span<const uint8_t> data) = 0;
    virtual ~Storage() = default;
};
