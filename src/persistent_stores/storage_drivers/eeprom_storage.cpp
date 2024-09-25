#include "eeprom_storage.hpp"

uint8_t EEPROMStorage::read_byte(uint16_t address) {
    return st25dv64k_user_read(address);
};
void EEPROMStorage::read_bytes(uint16_t address, std::span<uint8_t> buffer) {
    st25dv64k_user_read_bytes(address, buffer.data(), buffer.size());
};
void EEPROMStorage::write_byte(uint16_t address, uint8_t data) {
    st25dv64k_user_write(address, data);
};
void EEPROMStorage::write_bytes(uint16_t address, std::span<const uint8_t> data) {
    st25dv64k_user_write_bytes(address, data.data(), data.size());
};

void EEPROMStorage::erase_area(uint16_t start_address, uint16_t end_address) {
    static constexpr std::array<uint8_t, 4> data { 0xff, 0xff, 0xff, 0xff };
    for (; start_address < end_address; start_address += data.size()) {
        write_bytes(start_address, data);
    }
}
