#pragma once
#include <array>
#include <vector>
#include <stdint.h>
#include <span>
#include <storage_drivers/storage.hpp>

class DummyEepromChip : public configuration_store::Storage {
    std::array<uint8_t, 8096> data;

public:
    DummyEepromChip() {
        data.fill(0xff);
    }
    void set(uint16_t address, const uint8_t *pdata, uint16_t len);
    void set(uint16_t address, const uint8_t pdata);
    uint16_t set(uint16_t address, Bytes data);
    void get(uint16_t address, uint8_t *pdata, uint16_t len);
    uint8_t get(uint16_t address);
    std::span<uint8_t> get(uint16_t address, std::size_t size);
    void check_data(uint16_t address, std::vector<uint8_t> data);
    void check_data(uint16_t address, const uint8_t *data, std::size_t len);
    void clear();
    bool is_clear();
    size_t read_bytes(size_t address, WritableBytes buffer) override;
    size_t write_bytes(size_t address, Bytes data) override;
    void flush() override {}
};
extern DummyEepromChip eeprom_chip;

inline configuration_store::Storage &get_eeprom_chip() {
    return eeprom_chip;
}
