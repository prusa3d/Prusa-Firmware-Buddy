/**
 * @file eeprom_v4.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 4, from release 4.0.5-RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#pragma once
#include <stdint.h>
#include "constants.hpp"

namespace config_store_ns::old_eeprom::v4 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v4
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
    uint32_t CONNECT_IP4_ADDR;
    char CONNECT_TOKEN[old_eeprom::CONNECT_TOKEN_SIZE + 1];
    char LAN_HOSTNAME[old_eeprom::LAN_HOSTNAME_MAX_LEN + 1];
};

#pragma pack(pop)

} // namespace config_store_ns::old_eeprom::v4
