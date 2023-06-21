#pragma once
#include "storage.hpp"
#include "st25dv64k.h"

class EEPROMStorage : public configuration_store::Storage {
    // TODO detect errors and return them
public:
    virtual uint8_t read_byte(uint16_t address) {
        return st25dv64k_user_read(address);
    };
    virtual void read_bytes(uint16_t address, std::span<uint8_t> buffer) {
        st25dv64k_user_read_bytes(address, buffer.data(), buffer.size());
    };
    virtual void write_byte(uint16_t address, uint8_t data) {
        st25dv64k_user_write(address, data);
    };
    virtual void write_bytes(uint16_t address, std::span<const uint8_t> data) {
        st25dv64k_user_write_bytes(address, data.data(), data.size());
    };
};

inline configuration_store::Storage &EEPROMInstance() {
    static EEPROMStorage storage {};
    return storage;
}
