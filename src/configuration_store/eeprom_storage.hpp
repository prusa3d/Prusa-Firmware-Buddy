#pragma once
#include "storage.hpp"
#include "st25dv64k.h"

class EEPROMStorage : public configuration_store::Storage {
    // TODO detect errors and return them
public:
    uint8_t read_byte(uint16_t address) override;
    void read_bytes(uint16_t address, std::span<uint8_t> buffer) override;
    void write_byte(uint16_t address, uint8_t data) override;
    void write_bytes(uint16_t address, std::span<const uint8_t> data) override;
    void erase_area(uint16_t start_address, uint16_t end_address) override;
};

inline configuration_store::Storage &EEPROMInstance() {
    static EEPROMStorage storage {};
    return storage;
}
