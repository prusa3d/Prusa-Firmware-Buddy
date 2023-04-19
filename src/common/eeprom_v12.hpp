/**
 * @file eeprom_v12.hpp
 * @brief old version of eeprom, to be able to import it
 * version 12 from release 4.5.0
 */

#include "eeprom_v11.hpp"
#include "footer_eeprom.hpp"

namespace eeprom::v12 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v12
 * without head, padding and crc
 * eeprom setting storage has changed to using 5 bits pre item and last 3 bits are ones to force default footer setting on FW < 3.3.4
 */
struct vars_body_t : public eeprom::v11::vars_body_t {
    char CONNECT_HOST[CONNECT_HOST_SIZE + 1];
    char CONNECT_TOKEN[CONNECT_TOKEN_SIZE + 1];
    uint16_t CONNECT_PORT;
    bool CONNECT_TLS;
    bool CONNECT_ENABLED;
    uint16_t JOB_ID;
    int8_t CRASH_ENABLED;
    int16_t EEVAR_CRASH_SENS_X;
    int16_t EEVAR_CRASH_SENS_Y;
    uint16_t EEVAR_CRASH_MAX_PERIOD_X;
    uint16_t EEVAR_CRASH_MAX_PERIOD_Y;
    uint8_t EEVAR_CRASH_FILTER;
    uint16_t EEVAR_CRASH_COUNT_X_TOT;
    uint16_t EEVAR_CRASH_COUNT_Y_TOT;
    uint16_t EEVAR_POWER_COUNT_TOT;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(eeprom::v10::vars_body_t) + sizeof(uint8_t) * 3 + PL_PASSWORD_SIZE + LAN_HOSTNAME_MAX_LEN + 1 + WIFI_MAX_SSID_LEN + 1 + WIFI_MAX_PASSWD_LEN + 1 + 1 + 2 + 5 + 4 + 1 + 6 + sizeof(uint32_t) * 5 + sizeof(vars_body_t::CONNECT_HOST) + sizeof(vars_body_t::CONNECT_TOKEN) + sizeof(vars_body_t::CONNECT_PORT) + sizeof(vars_body_t::CONNECT_TLS) + sizeof(vars_body_t::CONNECT_ENABLED),
    "eeprom body size does not match");

static constexpr int crash_sens[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_STALL_GUARD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

static constexpr int crash_max_period[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_MAX_PERIOD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

static constexpr bool crash_filter =
#if ENABLED(CRASH_RECOVERY)
    CRASH_FILTER;
#else
    false;
#endif // ENABLED(CRASH_RECOVERY)

constexpr vars_body_t body_defaults = {
    eeprom::v11::body_defaults,
    "",                  // CONNECT_HOST
    "",                  // CONNECT_TOKEN
    80,                  // CONNECT_PORT
    true,                // CONNECT_TLS
    false,               // CONNECT_ENABLED
    0,                   // EEVAR_JOB_ID
    1,                   // EEVAR_CRASH_ENABLED
    crash_sens[0],       // EEVAR_CRASH_SENS_X,
    crash_sens[1],       // EEVAR_CRASH_SENS_Y,
    crash_max_period[0], // EEVAR_CRASH_MAX_PERIOD_X,
    crash_max_period[1], // EEVAR_CRASH_MAX_PERIOD_Y,
    crash_filter,        // EEVAR_CRASH_FILTER,
    0,                   // EEVAR_CRASH_COUNT_X_TOT
    0,                   // EEVAR_CRASH_COUNT_Y_TOT
    0,                   // EEVAR_POWER_COUNT_TOT
};

inline vars_body_t convert(const eeprom::v11::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v11 struct
    memcpy(&ret, &src, sizeof(eeprom::v11::vars_body_t));
    // count of eeprom items in last eeprom version is 6
    ret.FOOTER_SETTING = footer::eeprom::ConvertFromOldEeprom(ret.FOOTER_SETTING, 6);

    return ret;
}

} // namespace
