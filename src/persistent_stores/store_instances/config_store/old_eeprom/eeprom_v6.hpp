/**
 * @file eeprom_v6.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 6 from release 4.1.0-RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#pragma once
#include "eeprom_v4.hpp"
#include "config.h"
#include <cstring>

namespace config_store_ns::old_eeprom::v6 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v6
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
    char LAN_HOSTNAME[old_eeprom::LAN_HOSTNAME_MAX_LEN + 1];
    int8_t TIMEZONE;
    uint8_t SOUND_MODE;
};

#pragma pack(pop)

constexpr vars_body_t body_defaults = {
    0, // EEVAR_FILAMENT_TYPE
    0, // EEVAR_FILAMENT_COLOR
    true, // EEVAR_RUN_SELFTEST
    true, // EEVAR_RUN_XYZCALIB
    true, // EEVAR_RUN_FIRSTLAY
    true, // EEVAR_FSENSOR_ENABLED
    0, // EEVAR_ZOFFSET
#ifdef DEFAULT_Kp
    DEFAULT_Kp, // EEVAR_PID_NOZ_P
#else
    0,
#endif
#ifdef DEFAULT_Ki
    scalePID_i(DEFAULT_Ki), // EEVAR_PID_NOZ_I
#else
    0,
#endif
#ifdef DEFAULT_Kd
    scalePID_d(DEFAULT_Kd), // EEVAR_PID_NOZ_D
#else
    0,
#endif
#ifdef DEFAULT_bedKp
    DEFAULT_bedKp, // EEVAR_PID_BED_P
#else
    0,
#endif
#ifdef DEFAULT_bedKi
    scalePID_i(DEFAULT_bedKi), // EEVAR_PID_BED_I
#else
    0,
#endif
#ifdef DEFAULT_bedKd
    scalePID_d(DEFAULT_bedKd), // EEVAR_PID_BED_D
#else
    0,
#endif
    0, // EEVAR_LAN_FLAG
    0, // EEVAR_LAN_IP4_ADDR
    0, // EEVAR_LAN_IP4_MSK
    0, // EEVAR_LAN_IP4_GW
    0, // EEVAR_LAN_IP4_DNS1
    0, // EEVAR_LAN_IP4_DNS2
    LAN_HOSTNAME_DEF, // EEVAR_LAN_HOSTNAME
    1, // EEVAR_TIMEZONE
    0xff, // EEVAR_SOUND_MODE
};

inline vars_body_t convert(const old_eeprom::v4::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy range from FILAMENT_TYPE to LAN_IP4_DNS2
    size_t sz = ((uint8_t *)&src.CONNECT_IP4_ADDR) - ((uint8_t *)&src.FILAMENT_TYPE);
    memcpy(&ret, &src, sz);

    // copy LAN_HOSTNAME
    memcpy(&ret.LAN_HOSTNAME, &src.LAN_HOSTNAME, old_eeprom::LAN_HOSTNAME_MAX_LEN + 1);

    return ret;
}

} // namespace config_store_ns::old_eeprom::v6
