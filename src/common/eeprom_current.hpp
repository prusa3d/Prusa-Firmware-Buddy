/**
 * @file eeprom_current.hpp
 * @author Radek Vana
 * @brief current version of eeprom
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v10.hpp"

namespace eeprom::current {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v10
 * without head, padding and crc
 */
struct vars_body_t : public eeprom::v10::vars_body_t {
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
#if (EEPROM_FEATURES & EEPROM_FEATURE_CONNECT)
    char CONNECT_HOST[CONNECT_HOST_SIZE + 1];
    char CONNECT_TOKEN[CONNECT_TOKEN_SIZE + 1];
    uint16_t CONNECT_PORT;
    bool CONNECT_TLS;
    bool CONNECT_ENABLED;
#endif
    uint8_t USB_MSC_ENABLED;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(eeprom::v10::vars_body_t) + sizeof(uint8_t) * 3 + PL_API_KEY_SIZE + LAN_HOSTNAME_MAX_LEN + 1 + WIFI_MAX_SSID_LEN + 1 + WIFI_MAX_PASSWD_LEN + 1 + 1 + sizeof(uint32_t) * 5

#if (EEPROM_FEATURES & EEPROM_FEATURE_CONNECT)
            + sizeof(vars_body_t::CONNECT_HOST) + sizeof(vars_body_t::CONNECT_TOKEN) + sizeof(vars_body_t::CONNECT_PORT) + sizeof(vars_body_t::CONNECT_TLS) + sizeof(vars_body_t::CONNECT_ENABLED)
#endif
                  ,
    "eeprom body size does not match");

constexpr vars_body_t body_defaults = {
    eeprom::v10::body_defaults,
    0,                 // EEVAR_ACTIVE_NETDEV
    1,                 // EEVAR_PL_RUN
    "",                // EEVAR_PL_API_KEY
    0,                 // EEVAR_WIFI_FLAG
    0,                 // EEVAR_WIFI_IP4_ADDR
    0,                 // EEVAR_WIFI_IP4_MSK
    0,                 // EEVAR_WIFI_IP4_GW
    0,                 // EEVAR_WIFI_IP4_DNS1
    0,                 // EEVAR_WIFI_IP4_DNS2
    DEFAULT_HOST_NAME, // EEVAR_WIFI_HOSTNAME
    "",                // EEVAR_WIFI_AP_SSID
    "",                // EEVAR_WIFI_AP_PASSWD
#if (EEPROM_FEATURES & EEPROM_FEATURE_CONNECT)
    "",    // CONNECT_HOST
    "",    // CONNECT_TOKEN
    80,    // CONNECT_PORT
    true,  // CONNECT_TLS
    false, // CONNECT_ENABLED
#endif
    false, // EEVAR_USB_MSC_ENABLED
};

inline vars_body_t convert(const eeprom::v10::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v10 struct
    memcpy(&ret, &src, sizeof(eeprom::v10::vars_body_t));

    return ret;
}

} // namespace
