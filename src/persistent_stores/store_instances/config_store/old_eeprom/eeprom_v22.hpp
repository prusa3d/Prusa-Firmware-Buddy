/**
 * @file eeprom_v22.hpp
 * @brief version 22 from release 4.6.1
 */

#pragma once
#include "eeprom_v32789.hpp"
#include <utility_extensions.hpp>

namespace config_store_ns::old_eeprom::v22 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of current eeprom
 * without head and crc
 */
struct vars_body_t : public old_eeprom::v32789::vars_body_t {
    float ODOMETER_E1;
    float ODOMETER_E2;
    float ODOMETER_E3;
    float ODOMETER_E4;
    float ODOMETER_E5;
    uint32_t ODOMETER_T0;
    uint32_t ODOMETER_T1;
    uint32_t ODOMETER_T2;
    uint32_t ODOMETER_T3;
    uint32_t ODOMETER_T4;
    uint32_t ODOMETER_T5;
    uint8_t HWCHECK_COMPATIBILITY;
};

#pragma pack(pop)

constexpr vars_body_t body_defaults = {
    old_eeprom::v32789::body_defaults,
    0, // EEVAR_ODOMETER_E1
    0, // EEVAR_ODOMETER_E2
    0, // EEVAR_ODOMETER_E3
    0, // EEVAR_ODOMETER_E4
    0, // EEVAR_ODOMETER_E5
    0, // EEVAR_ODOMETER_T0
    0, // EEVAR_ODOMETER_T1
    0, // EEVAR_ODOMETER_T2
    0, // EEVAR_ODOMETER_T3
    0, // EEVAR_ODOMETER_T4
    0, // EEVAR_ODOMETER_T5
    1, // EEVAR_HWCHECK_COMPATIBILITY
};

// Explicitly two versions for MINI/XL without ifdefs inside to make sure this doesn't break in the future
#if PRINTER_IS_PRUSA_MINI()
enum class FooterItems : uint8_t {
    Nozzle,
    Bed,
    Filament,
    FSValue, // new
    FSensor,
    Speed,
    AxisX,
    AxisY,
    AxisZ,
    ZHeight,
    PrintFan,
    HeatbreakFan,
    LiveZ,
    Heatbreak,
    Sheets,
    None,
    _count
};

inline constexpr size_t old_footer_lines { v12::old_footer_lines };
inline constexpr size_t old_footer_items_per_line { v12::old_footer_items_per_line };
using Record = std::array<FooterItems, old_footer_items_per_line>;
inline constexpr Record default_items = { { FooterItems::Speed,
    FooterItems::ZHeight,
    FooterItems::Filament } };

inline constexpr size_t count = old_footer_items_per_line;
inline constexpr size_t count_of_trailing_ones = v12::count_of_trailing_ones;
inline constexpr size_t value_bit_size = v12::value_bit_size; // 32 different items should be enough

#elif PRINTER_IS_PRUSA_XL()

enum class FooterItems : uint8_t {
    Nozzle,
    Bed,
    Filament,
    FSValue, // new
    FSensor,
    Speed,
    AxisX,
    AxisY,
    AxisZ,
    ZHeight,
    PrintFan,
    HeatbreakFan,
    Heatbreak,
    CurrentTool,
    AllNozzles,
    FSensorSide,
    None,
    _count
};

inline constexpr size_t old_footer_lines { v32787::old_footer_lines };
inline constexpr size_t old_footer_items_per_line { v32787::old_footer_items_per_line };
using Record = std::array<FooterItems, old_footer_items_per_line>;
inline constexpr Record default_items = { { FooterItems::Nozzle,
    FooterItems::Bed,
    FooterItems::Filament,
    FooterItems::None,
    FooterItems::None } };

inline constexpr size_t count = old_footer_items_per_line;
inline constexpr size_t count_of_trailing_ones = v32787::count_of_trailing_ones;
inline constexpr size_t value_bit_size = v32787::value_bit_size; // 32 different items should be enough
#endif

#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_XL()
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
#endif

#if PRINTER_IS_PRUSA_MINI()
inline Record decode_from_v12(uint32_t encoded) {
    static constexpr size_t min_bit_size { value_bit_size };
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    Record ret = { {} };

    // version >= v11 has trailing ones
    encoded >>= count_of_trailing_ones;

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= ftrstd::to_underlying(FooterItems::_count)) {
            return default_items; // data corrupted, return default setting
        }

        auto previous_item = static_cast<v12::FooterItems>(decoded);
        switch (previous_item) { // easiest way to ensure there's no mistake is the most boring one - a switch case
        case v12::FooterItems::Nozzle:
            ret[i] = FooterItems::Nozzle;
            break;
        case v12::FooterItems::Bed:
            ret[i] = FooterItems::Bed;
            break;
        case v12::FooterItems::Filament:
            ret[i] = FooterItems::Filament;
            break;
        case v12::FooterItems::FSensor:
            ret[i] = FooterItems::FSensor;
            break;
        case v12::FooterItems::Speed:
            ret[i] = FooterItems::Speed;
            break;
        case v12::FooterItems::AxisX:
            ret[i] = FooterItems::AxisX;
            break;
        case v12::FooterItems::AxisY:
            ret[i] = FooterItems::AxisY;
            break;
        case v12::FooterItems::AxisZ:
            ret[i] = FooterItems::AxisZ;
            break;
        case v12::FooterItems::ZHeight:
            ret[i] = FooterItems::ZHeight;
            break;
        case v12::FooterItems::PrintFan:
            ret[i] = FooterItems::PrintFan;
            break;
        case v12::FooterItems::HeatbreakFan:
            ret[i] = FooterItems::HeatbreakFan;
            break;
        case v12::FooterItems::Heatbreak:
            ret[i] = FooterItems::Heatbreak;
            break;
        case v12::FooterItems::LiveZ:
            ret[i] = FooterItems::LiveZ;
            break;
        case v12::FooterItems::Sheets:
            ret[i] = FooterItems::Sheets;
            break;
        case v12::FooterItems::None:
        case v12::FooterItems::_count:
        default: // better to set the footer as missing than anything else in case of a random value
            ret[i] = FooterItems::None;
            break;
        }
        encoded >>= min_bit_size;
    }
    return ret;
}

inline uint32_t convert_from_old_eeprom(uint32_t encoded) {
    auto decoded = decode_from_v12(encoded);
    return encode(decoded);
}
#elif PRINTER_IS_PRUSA_XL()
inline Record decode_from_32787(uint32_t encoded) {
    static constexpr size_t min_bit_size { value_bit_size };
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    Record ret = { {} };

    // version >= v11 has trailing ones
    encoded >>= count_of_trailing_ones;

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= ftrstd::to_underlying(FooterItems::_count)) {
            return default_items; // data corrupted, return default setting
        }

        auto previous_item = static_cast<v32787::FooterItems>(decoded);
        switch (previous_item) { // easiest way to ensure there's no mistake is the most boring one - a switch case
        case v32787::FooterItems::Nozzle:
            ret[i] = FooterItems::Nozzle;
            break;
        case v32787::FooterItems::Bed:
            ret[i] = FooterItems::Bed;
            break;
        case v32787::FooterItems::Filament:
            ret[i] = FooterItems::Filament;
            break;
        case v32787::FooterItems::FSensor:
            ret[i] = FooterItems::FSensor;
            break;
        case v32787::FooterItems::Speed:
            ret[i] = FooterItems::Speed;
            break;
        case v32787::FooterItems::AxisX:
            ret[i] = FooterItems::AxisX;
            break;
        case v32787::FooterItems::AxisY:
            ret[i] = FooterItems::AxisY;
            break;
        case v32787::FooterItems::AxisZ:
            ret[i] = FooterItems::AxisZ;
            break;
        case v32787::FooterItems::ZHeight:
            ret[i] = FooterItems::ZHeight;
            break;
        case v32787::FooterItems::PrintFan:
            ret[i] = FooterItems::PrintFan;
            break;
        case v32787::FooterItems::HeatbreakFan:
            ret[i] = FooterItems::HeatbreakFan;
            break;
        case v32787::FooterItems::Heatbreak:
            ret[i] = FooterItems::Heatbreak;
            break;
        case v32787::FooterItems::CurrentTool:
            ret[i] = FooterItems::CurrentTool;
            break;
        case v32787::FooterItems::AllNozzles:
            ret[i] = FooterItems::AllNozzles;
            break;
        case v32787::FooterItems::FSensorSide:
            ret[i] = FooterItems::FSensorSide;
            break;
        case v32787::FooterItems::None:
        case v32787::FooterItems::_count:
        default: // better to set the footer as missing than anything else in case of a random value
            ret[i] = FooterItems::None;
            break;
        }
        encoded >>= min_bit_size;
    }
    return ret;
}

inline uint32_t convert_from_old_eeprom(uint32_t encoded) {
    auto decoded = decode_from_32787(encoded);
    return encode(decoded);
}

#endif

inline vars_body_t convert(const old_eeprom::v32789::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v32789 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v32789::vars_body_t));

#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_XL()
    // properly convert from previous version
    ret.FOOTER_SETTING = convert_from_old_eeprom(ret.FOOTER_SETTING);
#endif

    return ret;
}

} // namespace config_store_ns::old_eeprom::v22
