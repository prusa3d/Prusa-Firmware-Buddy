/**
 * @file eeprom_v9.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 9 from release 4.3.0 RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#pragma once
#include "eeprom_v7.hpp"
#include <common/sheet.hpp>
#include <limits>

namespace config_store_ns::old_eeprom::v9 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v9
 * without head, padding and crc
 */
struct vars_body_t : public old_eeprom::v7::vars_body_t {
    uint8_t FILE_SORT;
    uint8_t MENU_TIMEOUT;
    uint8_t ACTIVE_SHEET;
    Sheet SHEET_PROFILE0;
    Sheet SHEET_PROFILE1;
    Sheet SHEET_PROFILE2;
    Sheet SHEET_PROFILE3;
    Sheet SHEET_PROFILE4;
    Sheet SHEET_PROFILE5;
    Sheet SHEET_PROFILE6;
    Sheet SHEET_PROFILE7;
    uint32_t SELFTEST_RESULT_V1;
    uint8_t DEVHASH_IN_QR;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(old_eeprom::v7::vars_body_t) + sizeof(vars_body_t::FILE_SORT) + sizeof(vars_body_t::MENU_TIMEOUT) + sizeof(vars_body_t::ACTIVE_SHEET) + 8 * sizeof(Sheet) + sizeof(vars_body_t::SELFTEST_RESULT_V1) + sizeof(vars_body_t::DEVHASH_IN_QR),
    "eeprom body size does not match");

inline constexpr float eeprom_z_offset_uncalibrated { std::numeric_limits<float>::max() };

constexpr vars_body_t body_defaults = {
    old_eeprom::v7::body_defaults,
    0, // EEVAR_FILE_SORT
    true, // EEVAR_MENU_TIMEOUT
    0, // EEVAR_ACTIVE_SHEET
    { "Smooth1", 0.0f }, // EEVAR_SHEET_PROFILE0
    { "Smooth2", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE1
    { "Textur1", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE2
    { "Textur2", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE3
    { "Custom1", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE4
    { "Custom2", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE5
    { "Custom3", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE6
    { "Custom4", eeprom_z_offset_uncalibrated }, // EEVAR_SHEET_PROFILE_LAST
    0, // EEVAR_SELFTEST_RESULT
    true, // EEVAR_DEVHASH_IN_QR
};

inline vars_body_t convert(const old_eeprom::v7::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v7 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v7::vars_body_t));

    return ret;
}

} // namespace config_store_ns::old_eeprom::v9
