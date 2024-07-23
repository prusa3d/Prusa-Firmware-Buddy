/**
 * @file eeprom_v11.hpp
 * @brief old version of eeprom, to be able to import it
 * version 11 from release 4.4.0-rc1
 */

#pragma once
#include "eeprom_v10.hpp"
#include <utility_extensions.hpp>

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
    0, // EEVAR_ACTIVE_NETDEV
    1, // EEVAR_PL_RUN
    "", // EEVAR_PL_PASSWORD
    0, // EEVAR_WIFI_FLAG
    0, // EEVAR_WIFI_IP4_ADDR
    0, // EEVAR_WIFI_IP4_MSK
    0, // EEVAR_WIFI_IP4_GW
    0, // EEVAR_WIFI_IP4_DNS1
    0, // EEVAR_WIFI_IP4_DNS2
    LAN_HOSTNAME_DEF, // EEVAR_WIFI_HOSTNAME
    "", // EEVAR_WIFI_AP_SSID
    "", // EEVAR_WIFI_AP_PASSWD
    false, // EEVAR_USB_MSC_ENABLED
};

// Between v10 and v11 some items were added to the footer, so record new 'snapshot' and a have a migration function from v10

#if PRINTER_IS_PRUSA_MINI()
enum class FooterItems : uint8_t {
    Nozzle,
    Bed,
    Filament,
    FSensor, // new
    Speed,
    AxisX, // new
    AxisY, // new
    AxisZ, // new
    ZHeight, // new
    PrintFan, // new
    HeatbreakFan, // new
    LiveZ,
    Sheets,
    None,
    _count
};

inline constexpr size_t old_footer_lines { 2 };
inline constexpr size_t old_footer_items_per_line { 3 };
using Record = std::array<FooterItems, old_footer_items_per_line>;
inline constexpr Record default_items = { { FooterItems::Speed,
    FooterItems::ZHeight,
    FooterItems::Filament } };

inline constexpr size_t count = old_footer_items_per_line;
inline constexpr size_t count_of_trailing_ones = 3;
inline constexpr size_t value_bit_size = 5; // 32 different items should be enough

/**
 * @brief encodes footer setting to uint32_t
 *
 * @param rec footer setting to encode
 * @return constexpr uint32_t
 */
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

inline Record decode_from_v10(uint32_t encoded) {
    // count of eeprom items in last eeprom version v10 is 6, which is encoded in 3 bytes
    static constexpr size_t min_bit_size { 3 };
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    Record ret = { {} };

    // v10 is encoded without trailing ones

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= ftrstd::to_underlying(FooterItems::_count)) {
            return default_items; // data corrupted, return default setting
        }

        auto previous_item = static_cast<v10::FooterItems>(decoded);
        switch (previous_item) { // easiest way to ensure there's no mistake is the most boring one - a switch case
        case v10::FooterItems::ItemNozzle:
            ret[i] = FooterItems::Nozzle;
            break;
        case v10::FooterItems::ItemBed:
            ret[i] = FooterItems::Bed;
            break;
        case v10::FooterItems::ItemFilament:
            ret[i] = FooterItems::Filament;
            break;
        case v10::FooterItems::ItemSpeed:
            ret[i] = FooterItems::Speed;
            break;
        case v10::FooterItems::ItemLiveZ:
            ret[i] = FooterItems::LiveZ;
            break;
        case v10::FooterItems::ItemSheets:
            ret[i] = FooterItems::Sheets;
            break;
        case v10::FooterItems::count_: // count_ is not the same thing as _count, previous count_ is what now is None :)
        default: // better to set the footer as missing than anything else in case of a random value
            ret[i] = FooterItems::None;
            break;
        }
        encoded >>= min_bit_size;
    }
    return ret;
}

inline uint32_t convert_from_old_eeprom(uint32_t encoded) {
    auto decoded = decode_from_v10(encoded);
    return encode(decoded);
}
#endif

inline vars_body_t convert(const old_eeprom::v10::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v10 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v10::vars_body_t));
#if PRINTER_IS_PRUSA_MINI()
    ret.FOOTER_SETTING = convert_from_old_eeprom(ret.FOOTER_SETTING);
#endif // other printers don't exist yet

    return ret;
}

} // namespace config_store_ns::old_eeprom::v11
