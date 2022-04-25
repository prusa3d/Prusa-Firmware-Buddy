/**
 * @file eeprom_v9.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 9 from release 4.3.0 RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v7.hpp"

namespace eeprom::v9 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v9
 * without head, padding and crc
 */
struct vars_body_t : public eeprom::v7::vars_body_t {
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
    uint32_t SELFTEST_RESULT;
    uint8_t DEVHASH_IN_QR;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(eeprom::v7::vars_body_t) + sizeof(vars_body_t::FILE_SORT) + sizeof(vars_body_t::MENU_TIMEOUT) + sizeof(vars_body_t::ACTIVE_SHEET) + 8 * sizeof(Sheet) + sizeof(vars_body_t::SELFTEST_RESULT) + sizeof(vars_body_t::DEVHASH_IN_QR), "eeprom body size does not match");

constexpr vars_body_t body_defaults = {
    eeprom::v7::body_defaults,
    0,    // EEVAR_FILE_SORT
    true, // EEVAR_MENU_TIMEOUT
    0,    // EEVAR_ACTIVE_SHEET
    { "Smooth1", 0.0f },
    { "Smooth2", eeprom_z_offset_uncalibrated },
    { "Textur1", eeprom_z_offset_uncalibrated },
    { "Textur2", eeprom_z_offset_uncalibrated },
    { "Custom1", eeprom_z_offset_uncalibrated },
    { "Custom2", eeprom_z_offset_uncalibrated },
    { "Custom3", eeprom_z_offset_uncalibrated },
    { "Custom4", eeprom_z_offset_uncalibrated },
    0,    // EEVAR_SELFTEST_RESULT
    true, // EEVAR_DEVHASH_IN_QR
};

inline vars_body_t convert(const eeprom::v7::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v7 struct
    memcpy(&ret, &src, sizeof(eeprom::v7::vars_body_t));

    return ret;
}

} // namespace
