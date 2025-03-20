#pragma once

#include "filament.hpp"

// For historic reasons, the FilamentTypeParameters is split across multiple structures in the EEPROM

// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters_EEPROM1 {

public:
    std::array<char, 8> name { '\0' };

    uint16_t nozzle_temperature;
    uint16_t nozzle_preheat_temperature = 170;
    uint8_t heatbed_temperature;

    bool requires_filtration : 1 = false;
    bool is_abrasive : 1 = false;
    bool is_flexible : 1 = false;

    // Keeping the remaining bits of the bitfield unused, but zero initizliazed, for future proofing
    uint8_t _unused : 5 = 0;

public:
    constexpr bool operator==(const FilamentTypeParameters_EEPROM1 &) const = default;
    constexpr bool operator!=(const FilamentTypeParameters_EEPROM1 &) const = default;
};

// !!! DO NOT CHANGE - this is used in config store
struct __attribute__((packed)) FilamentTypeParameters_EEPROM2 {

public:
    static constexpr uint8_t chamber_temp_off = 255;

    uint8_t chamber_min_temperature = chamber_temp_off;
    uint8_t chamber_max_temperature = chamber_temp_off;
    uint8_t chamber_target_temperature = chamber_temp_off;

public:
    static constexpr std::optional<uint8_t> decode_chamber_temp(uint8_t val) {
        return val == chamber_temp_off ? std::optional<uint8_t>() : val;
    }
    static constexpr uint8_t encode_chamber_temp(std::optional<uint8_t> val) {
        return val.value_or(chamber_temp_off);
    }

    constexpr bool operator==(const FilamentTypeParameters_EEPROM2 &) const = default;
    constexpr bool operator!=(const FilamentTypeParameters_EEPROM2 &) const = default;
};
