/**
 * @file eeprom_v11.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 11 from TODO add after released
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v10.hpp"

namespace eeprom::v11 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body od eeprom v10
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
    uint32_t FOOTER_SETTING;
    uint32_t FOOTER_DRAW_TYPE;
    uint8_t FAN_CHECK_ENABLED;
    uint8_t FS_AUTOLOAD_ENABLED;
    float EEVAR_ODOMETER_X;
    float EEVAR_ODOMETER_Y;
    float EEVAR_ODOMETER_Z;
    float EEVAR_ODOMETER_E0;
    float AXIS_STEPS_PER_UNIT_X;
    float AXIS_STEPS_PER_UNIT_Y;
    float AXIS_STEPS_PER_UNIT_Z;
    float AXIS_STEPS_PER_UNIT_E0;
    uint16_t AXIS_MICROSTEPS_X;
    uint16_t AXIS_MICROSTEPS_Y;
    uint16_t AXIS_MICROSTEPS_Z;
    uint16_t AXIS_MICROSTEPS_E0;
    uint16_t AXIS_RMS_CURRENT_MA_X;
    uint16_t AXIS_RMS_CURRENT_MA_Y;
    uint16_t AXIS_RMS_CURRENT_MA_Z;
    uint16_t AXIS_RMS_CURRENT_MA_E0;
    float AXIS_Z_MAX_POS_MM;
    uint32_t ODOMETER_TIME;
    uint8_t EEVAR_ACTIVE_NETDEV;
    uint8_t EEVAR_PL_RUN;
    char EEVAR_PL_API_KEY[PL_API_KEY_SIZE];
    uint8_t WIFI_FLAG;
    uint32_t WIFI_IP4_ADDR;
    uint32_t WIFI_IP4_MSK;
    uint32_t WIFI_IP4_GW;
    uint32_t WIFI_IP4_DNS1;
    uint32_t WIFI_IP4_DNS2;
    char WIFI_HOSTNAME[LAN_HOSTNAME_MAX_LEN + 1];
    char WIFI_AP_SSID[WIFI_MAX_SSID_LEN + 1];
    char WIFI_AP_PASSWD[WIFI_MAX_PASSWD_LEN + 1];
};

#pragma pack(pop)

static constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;

constexpr vars_body_t body_defaults = {
    0,                         // EEVAR_FILAMENT_TYPE
    0,                         // EEVAR_FILAMENT_COLOR
    1,                         // EEVAR_RUN_SELFTEST
    1,                         // EEVAR_RUN_XYZCALIB
    1,                         // EEVAR_RUN_FIRSTLAY
    1,                         // EEVAR_FSENSOR_ENABLED
    0,                         // EEVAR_ZOFFSET_DO_NOT_USE_DIRECTLY
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
    0,                                                                          // EEVAR_SELFTEST_RESULT
    1,                                                                          // EEVAR_DEVHASH_IN_QR
    footer::eeprom::Encode(footer::DefaultItems),                               // EEVAR_FOOTER_SETTING
    uint32_t(footer::ItemDrawCnf::Default()),                                   // EEVAR_FOOTER_DRAW_TYPE
    1,                                                                          // EEVAR_FAN_CHECK_ENABLED
    0,                                                                          // EEVAR_FS_AUTOLOAD_ENABLED
    0,                                                                          // EEVAR_ODOMETER_X
    0,                                                                          // EEVAR_ODOMETER_Y
    0,                                                                          // EEVAR_ODOMETER_Z
    0,                                                                          // EEVAR_ODOMETER_E0
    default_axis_steps_flt[0] * ((DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_X
    default_axis_steps_flt[1] * ((DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Y
    default_axis_steps_flt[2] * ((DEFAULT_INVERT_Z_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Z
    default_axis_steps_flt[3] * ((DEFAULT_INVERT_E0_DIR == true) ? -1.f : 1.f), // AXIS_STEPS_PER_UNIT_E0
    X_MICROSTEPS,                                                               // AXIS_MICROSTEPS_X
    Y_MICROSTEPS,                                                               // AXIS_MICROSTEPS_Y
    Z_MICROSTEPS,                                                               // AXIS_MICROSTEPS_Z
    E0_MICROSTEPS,                                                              // AXIS_MICROSTEPS_E0
    X_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_X
    Y_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_Y
    Z_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_Z
    E0_CURRENT,                                                                 // AXIS_RMS_CURRENT_MA_E0
    DEFAULT_Z_MAX_POS,                                                          // AXIS_Z_MAX_POS_MM
    0,                                                                          // EEVAR_ODOMETER_TIME
    0,                                                                          // EEVAR_ACTIVE_NETDEV
    1,                                                                          // EEVAR_PL_RUN
    "",                                                                         // EEVAR_PL_API_KEY
    0,                                                                          // EEVAR_WIFI_FLAG
    0,                                                                          // EEVAR_WIFI_IP4_ADDR
    0,                                                                          // EEVAR_WIFI_IP4_MSK
    0,                                                                          // EEVAR_WIFI_IP4_GW
    0,                                                                          // EEVAR_WIFI_IP4_DNS1
    0,                                                                          // EEVAR_WIFI_IP4_DNS2
    DEFAULT_HOST_NAME,                                                          // EEVAR_WIFI_HOSTNAME
    "",                                                                         // EEVAR_WIFI_AP_SSID
    "",                                                                         // EEVAR_WIFI_AP_PASSWD
};

inline vars_body_t convert(const eeprom::v10::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v9 struct
    memcpy(&ret, &src, sizeof(eeprom::v10::vars_body_t));

    return ret;
}

} // namespace
