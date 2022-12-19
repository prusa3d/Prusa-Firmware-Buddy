#pragma once
#include <array>
#include <vector>
#include <stdint.h>

class DummyEepromChip {
    std::array<uint8_t, 8096> data;

public:
    DummyEepromChip() {
        data.fill(0xff);
    }
    void set(uint16_t address, const uint8_t *pdata, uint16_t len);
    void set(uint16_t address, const uint8_t pdata);
    void get(uint16_t address, uint8_t *pdata, uint16_t len);
    uint8_t get(uint16_t address);
    void check_data(uint16_t address, std::vector<uint8_t> data);
    void clear();
    bool is_clear();
};
extern DummyEepromChip eeprom_chip;
