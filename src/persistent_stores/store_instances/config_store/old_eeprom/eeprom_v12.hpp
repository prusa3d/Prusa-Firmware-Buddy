/**
 * @file eeprom_v12.hpp
 * @brief old version of eeprom, to be able to import it
 * version 12 from release 4.5.0
 */

#pragma once
#include "eeprom_v11.hpp"
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <utility_extensions.hpp>

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

inline constexpr int crash_sens[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_STALL_GUARD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

inline constexpr int crash_max_period[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_MAX_PERIOD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

inline constexpr bool crash_filter =
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
#if (( PRINTER_IS_PRUSA_MK4()) || ( PRINTER_IS_PRUSA_MK3_5()))
    false,               // EEVAR_CRASH_ENABLED
#else
    true,                // EEVAR_CRASH_ENABLED
#endif // (( PRINTER_IS_PRUSA_MK4()) || ( PRINTER_IS_PRUSA_MK3_5()))
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

#if PRINTER_IS_PRUSA_MINI()
enum class FooterItems : uint8_t {
    Nozzle,
    Bed,
    Filament,
    FSensor,
    Speed,
    AxisX,
    AxisY,
    AxisZ,
    ZHeight,
    PrintFan,
    HeatbreakFan,
    LiveZ,
    Heatbreak, // new
    Sheets,
    None,
    _count
};

inline constexpr size_t old_footer_lines { v11::old_footer_lines };
inline constexpr size_t old_footer_items_per_line { v11::old_footer_items_per_line };
using Record = std::array<FooterItems, old_footer_items_per_line>;
inline constexpr Record default_items = { { FooterItems::Speed,
    FooterItems::ZHeight,
    FooterItems::Filament } };

inline constexpr size_t count = old_footer_items_per_line;
inline constexpr size_t count_of_trailing_ones = 3;
inline constexpr size_t value_bit_size = 5; // 32 different items should be enough

constexpr uint32_t encode(Record rec) {
    uint32_t ret = uint32_t(rec[0]) << count_of_trailing_ones;
    for (size_t i = 1; i < count; ++i) {
        ret |= uint32_t(rec[i]) << ((value_bit_size * i) + count_of_trailing_ones);
    }
    // adding trailing ones to force default footer settings in FW version < 4.4.0 and using fixed size of footer item
    uint32_t trailing_ones = (1 << count_of_trailing_ones) - 1;
    ret |= trailing_ones;
    return ret;
}

inline Record decode_from_v11(uint32_t encoded) {
    static constexpr size_t min_bit_size { v11::value_bit_size };
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    Record ret = { {} };

    // version >= v11 has trailing ones
    encoded >>= count_of_trailing_ones;

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= ftrstd::to_underlying(FooterItems::_count)) {
            return default_items; // data corrupted, return default setting
        }

        auto previous_item = static_cast<v11::FooterItems>(decoded);
        switch (previous_item) { // easiest way to ensure there's no mistake is the most boring one - a switch case

        case v11::FooterItems::Nozzle:
            ret[i] = FooterItems::Nozzle;
            break;
        case v11::FooterItems::Bed:
            ret[i] = FooterItems::Bed;
            break;
        case v11::FooterItems::Filament:
            ret[i] = FooterItems::Filament;
            break;
        case v11::FooterItems::FSensor:
            ret[i] = FooterItems::FSensor;
            break;
        case v11::FooterItems::Speed:
            ret[i] = FooterItems::Speed;
            break;
        case v11::FooterItems::AxisX:
            ret[i] = FooterItems::AxisX;
            break;
        case v11::FooterItems::AxisY:
            ret[i] = FooterItems::AxisY;
            break;
        case v11::FooterItems::AxisZ:
            ret[i] = FooterItems::AxisZ;
            break;
        case v11::FooterItems::ZHeight:
            ret[i] = FooterItems::ZHeight;
            break;
        case v11::FooterItems::PrintFan:
            ret[i] = FooterItems::PrintFan;
            break;
        case v11::FooterItems::HeatbreakFan:
            ret[i] = FooterItems::HeatbreakFan;
            break;
        case v11::FooterItems::LiveZ:
            ret[i] = FooterItems::LiveZ;
            break;
        case v11::FooterItems::Sheets:
            ret[i] = FooterItems::Sheets;
            break;
        case v11::FooterItems::None:
        case v11::FooterItems::_count:
        default: // better to set the footer as missing than anything else in case of a random value
            ret[i] = FooterItems::None;
            break;
        }
        encoded >>= min_bit_size;
    }
    return ret;
}

inline uint32_t convert_from_old_eeprom(uint32_t encoded) {
    // count of eeprom items in last eeprom version v11 has the same value_bit_size
    auto decoded = decode_from_v11(encoded);
    return encode(decoded);
}
#endif

inline vars_body_t convert(const old_eeprom::v11::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v11 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v11::vars_body_t));

#if PRINTER_IS_PRUSA_MINI()
    // MINI needs to properly keep converting
    ret.FOOTER_SETTING = convert_from_old_eeprom(ret.FOOTER_SETTING);
#endif // else we can leave whatever values in, they won't be migrated to config_store (footer will be set to defaults)

    return ret;
}

} // namespace config_store_ns::old_eeprom::v12
