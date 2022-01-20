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
 * @brief body od eeprom v9
 * without head, padding and crc
 */
struct vars_body_t : public eeprom::v7::vars_body_t {
    constexpr vars_body_t()
        : FILE_SORT(0)
        , MENU_TIMEOUT(1)
        , ACTIVE_SHEET(0)
        , SHEET_PROFILE0 { "Smooth1", 0.0f }
        , SHEET_PROFILE1 { "Smooth2", FLT_MAX }
        , SHEET_PROFILE2 { "Textur1", FLT_MAX }
        , SHEET_PROFILE3 { "Textur2", FLT_MAX }
        , SHEET_PROFILE4 { "Custom1", FLT_MAX }
        , SHEET_PROFILE5 { "Custom2", FLT_MAX }
        , SHEET_PROFILE6 { "Custom3", FLT_MAX }
        , SHEET_PROFILE7 { "Custom4", FLT_MAX }
        , SELFTEST_RESULT(0)
        , DEVHASH_IN_QR(1) {}
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

static vars_body_t convert(const eeprom::v7::vars_body_t &src) {
    vars_body_t ret = vars_body_t();

    // copy entire v7 struct
    memcpy(&ret, &src, sizeof(eeprom::v7::vars_body_t));

    return ret;
}

} // namespace
