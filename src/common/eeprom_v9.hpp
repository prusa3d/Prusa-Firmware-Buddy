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
struct vars_body_t {
    uint8_t FILAMENT_TYPE;
    uint32_t FILAMENT_COLOR;
    uint8_t RUN_SELFTEST;
    uint8_t RUN_XYZCALIB;
    uint8_t RUN_FIRSTLAY;
    uint8_t FSENSOR_ENABLED;
    float ZOFFSET;
    float PID_NOZ_P;
    float PID_NOZ_I;
    float PID_NOZ_D;
    float PID_BED_P;
    float PID_BED_I;
    float PID_BED_D;
    uint8_t LAN_FLAG;
    uint32_t LAN_IP4_ADDR;
    uint32_t LAN_IP4_MSK;
    uint32_t LAN_IP4_GW;
    uint32_t LAN_IP4_DNS1;
    uint32_t LAN_IP4_DNS2;
    char LAN_HOSTNAME[LAN_HOSTNAME_MAX_LEN + 1];
    int8_t TIMEZONE;
    uint8_t SOUND_MODE;
    uint8_t SOUND_VOLUME;
    uint16_t LANGUAGE;
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

constexpr vars_body_t body_defaults = {
    0,                         // EEVAR_FILAMENT_TYPE
    0,                         // EEVAR_FILAMENT_COLOR
    1,                         // EEVAR_RUN_SELFTEST
    1,                         // EEVAR_RUN_XYZCALIB
    1,                         // EEVAR_RUN_FIRSTLAY
    1,                         // EEVAR_FSENSOR_ENABLED
    0,                         // EEVAR_ZOFFSET
    DEFAULT_Kp,                // EEVAR_PID_NOZ_P
    scalePID_i(DEFAULT_Ki),    // EEVAR_PID_NOZ_I
    scalePID_d(DEFAULT_Kd),    // EEVAR_PID_NOZ_D
    DEFAULT_bedKp,             // EEVAR_PID_BED_P
    scalePID_i(DEFAULT_bedKi), // EEVAR_PID_BED_I
    scalePID_d(DEFAULT_bedKd), // EEVAR_PID_BED_D
    0,                         // EEVAR_LAN_FLAG
    0,                         // EEVAR_LAN_IP4_ADDR
    0,                         // EEVAR_LAN_IP4_MSK
    0,                         // EEVAR_LAN_IP4_GW
    0,                         // EEVAR_LAN_IP4_DNS1
    0,                         // EEVAR_LAN_IP4_DNS2
    DEFAULT_HOST_NAME,         // EEVAR_LAN_HOSTNAME
    0,                         // EEVAR_TIMEZONE
    0xff,                      // EEVAR_SOUND_MODE
    5,                         // EEVAR_SOUND_VOLUME
    0xffff,                    // EEVAR_LANGUAGE
    0,                         // EEVAR_FILE_SORT
    1,                         // EEVAR_MENU_TIMEOUT
    0,                         // EEVAR_ACTIVE_SHEET
    { "Smooth1", 0.0f },
    { "Smooth2", FLT_MAX },
    { "Textur1", FLT_MAX },
    { "Textur2", FLT_MAX },
    { "Custom1", FLT_MAX },
    { "Custom2", FLT_MAX },
    { "Custom3", FLT_MAX },
    { "Custom4", FLT_MAX },
    0, // EEVAR_SELFTEST_RESULT
    1, // EEVAR_DEVHASH_IN_QR
};

inline vars_body_t convert(const eeprom::v7::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v7 struct
    memcpy(&ret, &src, sizeof(eeprom::v7::vars_body_t));

    return ret;
}

} // namespace
