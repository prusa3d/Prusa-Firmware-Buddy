/**
 * @file eeprom_v6.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 6 from release 4.1.0-RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v4.hpp"

namespace eeprom::v6 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body od eeprom v6
 * without head, padding and crc
 */
struct vars_body_t {
    constexpr vars_body_t()
        : FILAMENT_TYPE(0)
        , FILAMENT_COLOR(0)
        , RUN_SELFTEST(1)
        , RUN_XYZCALIB(1)
        , RUN_FIRSTLAY(1)
        , FSENSOR_ENABLED(1)
        , ZOFFSET(0)
        , PID_NOZ_P(DEFAULT_Kp)
        , PID_NOZ_I(scalePID_i(DEFAULT_Ki))
        , PID_NOZ_D(scalePID_d(DEFAULT_Kd))
        , PID_BED_P(DEFAULT_bedKp)
        , PID_BED_I(scalePID_i(DEFAULT_bedKi))
        , PID_BED_D(0)
        , LAN_FLAG(0)
        , LAN_IP4_ADDR(0)
        , LAN_IP4_MSK(0)
        , LAN_IP4_GW(0)
        , LAN_IP4_DNS1(0)
        , LAN_IP4_DNS2(0)
        , LAN_HOSTNAME(DEFAULT_HOST_NAME)
        , TIMEZONE(0)
        , SOUND_MODE(0xff) {}
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
};

#pragma pack(pop)

static vars_body_t convert(const eeprom::v4::vars_body_t &src) {
    vars_body_t ret = vars_body_t();

    // copy range from FILAMENT_TYPE to LAN_IP4_DNS2
    size_t sz = ((uint8_t *)&src.CONNECT_IP4_ADDR) - ((uint8_t *)&src.FILAMENT_TYPE);
    memcpy(&ret, &src, sz);

    // copy LAN_HOSTNAME
    memcpy(&ret.LAN_HOSTNAME, &src.LAN_HOSTNAME, LAN_HOSTNAME_MAX_LEN + 1);

    return ret;
}

} // namespace
