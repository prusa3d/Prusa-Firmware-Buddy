/**
 * @file eeprom_v12.hpp
 * @brief old version of eeprom, to be able to import it
 * version 12 from release 4.5.0
 */

#pragma once
#include "eeprom_v11.hpp"
#include "footer_eeprom.hpp"
#include <Marlin/src/inc/MarlinConfigPre.h>

namespace config_store_ns::old_eeprom::v12 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v12
 * without head, padding and crc
 * eeprom setting storage has changed to using 5 bits pre item and last 3 bits are ones to force default footer setting on FW < 3.3.4
 */
struct vars_body_t : public old_eeprom::v11::vars_body_t {
    char CONNECT_HOST[old_eeprom::CONNECT_HOST_SIZE + 1];
    char CONNECT_TOKEN[old_eeprom::CONNECT_TOKEN_SIZE + 1];
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

static_assert(sizeof(vars_body_t) == sizeof(old_eeprom::v10::vars_body_t) + sizeof(uint8_t) * 3 + old_eeprom::PL_PASSWORD_SIZE + old_eeprom::LAN_HOSTNAME_MAX_LEN + 1 + old_eeprom::WIFI_MAX_SSID_LEN + 1 + old_eeprom::WIFI_MAX_PASSWD_LEN + 1 + 1 + 2 + 5 + 4 + 1 + 6 + sizeof(uint32_t) * 5 + sizeof(vars_body_t::CONNECT_HOST) + sizeof(vars_body_t::CONNECT_TOKEN) + sizeof(vars_body_t::CONNECT_PORT) + sizeof(vars_body_t::CONNECT_TLS) + sizeof(vars_body_t::CONNECT_ENABLED),
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

// clang-format off
constexpr vars_body_t body_defaults = {
    old_eeprom::v11::body_defaults,
    // "Compressed" - this means buddy-a.connect.prusa3d.com.
    "buddy-a.\x01\x01",  // EEVAR_CONNECT_HOST
    "",                  // EEVAR_CONNECT_TOKEN
    443,                 // EEVAR_CONNECT_PORT
    true,                // EEVAR_CONNECT_TLS
    false,               // EEVAR_CONNECT_ENABLED
    0,                   // EEVAR_JOB_ID
#if (( PRINTER_IS_PRUSA_MK4) || ( PRINTER_IS_PRUSA_MK3_5))
    false,               // EEVAR_CRASH_ENABLED
#else
    true,                // EEVAR_CRASH_ENABLED
#endif // (( PRINTER_IS_PRUSA_MK4) || ( PRINTER_IS_PRUSA_MK3_5))
    crash_sens[0],       // EEVAR_CRASH_SENS_X,
    crash_sens[1],       // EEVAR_CRASH_SENS_Y,
    crash_max_period[0], // EEVAR_CRASH_MAX_PERIOD_X,
    crash_max_period[1], // EEVAR_CRASH_MAX_PERIOD_Y,
    crash_filter,        // EEVAR_CRASH_FILTER,
    0,                   // EEVAR_CRASH_COUNT_X_TOT
    0,                   // EEVAR_CRASH_COUNT_Y_TOT
    0,                   // EEVAR_POWER_COUNT_TOT
};
// clang-format on

inline vars_body_t convert(const old_eeprom::v11::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v11 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v11::vars_body_t));
    // count of eeprom items in last eeprom version is 6
    ret.FOOTER_SETTING = footer::eeprom::ConvertFromOldEeprom(ret.FOOTER_SETTING, 6);

    return ret;
}

} // namespace config_store_ns::old_eeprom::v12
