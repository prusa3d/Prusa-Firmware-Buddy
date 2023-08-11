/**
 * @file eeprom_v11.hpp
 * @brief old version of eeprom, to be able to import it
 * version 11 from release 4.4.0-rc1
 */

#pragma once
#include "eeprom_v10.hpp"

namespace config_store_ns::old_eeprom::v11 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v10
 * without head, padding and crc
 */
struct vars_body_t : public old_eeprom::v10::vars_body_t {
    uint8_t EEVAR_ACTIVE_NETDEV;
    uint8_t EEVAR_PL_RUN;
    char EEVAR_PL_PASSWORD[old_eeprom::PL_PASSWORD_SIZE];
    uint8_t WIFI_FLAG;
    uint32_t WIFI_IP4_ADDR;
    uint32_t WIFI_IP4_MSK;
    uint32_t WIFI_IP4_GW;
    uint32_t WIFI_IP4_DNS1;
    uint32_t WIFI_IP4_DNS2;
    char WIFI_HOSTNAME[old_eeprom::LAN_HOSTNAME_MAX_LEN + 1];
    char WIFI_AP_SSID[old_eeprom::WIFI_MAX_SSID_LEN + 1];
    char WIFI_AP_PASSWD[old_eeprom::WIFI_MAX_PASSWD_LEN + 1];
    uint8_t USB_MSC_ENABLED;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(old_eeprom::v10::vars_body_t) + sizeof(uint8_t) * 3 + old_eeprom::PL_PASSWORD_SIZE + old_eeprom::LAN_HOSTNAME_MAX_LEN + 1 + old_eeprom::WIFI_MAX_SSID_LEN + 1 + old_eeprom::WIFI_MAX_PASSWD_LEN + 1 + 1 + sizeof(uint32_t) * 5, "eeprom body size does not match");

constexpr vars_body_t body_defaults = {
    old_eeprom::v10::body_defaults,
    0,                // EEVAR_ACTIVE_NETDEV
    1,                // EEVAR_PL_RUN
    "",               // EEVAR_PL_PASSWORD
    0,                // EEVAR_WIFI_FLAG
    0,                // EEVAR_WIFI_IP4_ADDR
    0,                // EEVAR_WIFI_IP4_MSK
    0,                // EEVAR_WIFI_IP4_GW
    0,                // EEVAR_WIFI_IP4_DNS1
    0,                // EEVAR_WIFI_IP4_DNS2
    LAN_HOSTNAME_DEF, // EEVAR_WIFI_HOSTNAME
    "",               // EEVAR_WIFI_AP_SSID
    "",               // EEVAR_WIFI_AP_PASSWD
    false,            // EEVAR_USB_MSC_ENABLED
};

inline vars_body_t convert(const old_eeprom::v10::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v10 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v10::vars_body_t));

    return ret;
}

} // namespace config_store_ns::old_eeprom::v11
