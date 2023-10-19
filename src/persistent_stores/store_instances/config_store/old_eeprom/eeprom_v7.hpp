/**
 * @file eeprom_v7.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 7 from release 4.2.0-RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#pragma once
#include "eeprom_v6.hpp"

namespace config_store_ns::old_eeprom::v7 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v7
 * without head, padding and crc
 */
struct vars_body_t : public old_eeprom::v6::vars_body_t {
    uint8_t SOUND_VOLUME;
    uint16_t LANGUAGE;
};
#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(old_eeprom::v6::vars_body_t) + sizeof(vars_body_t::SOUND_VOLUME) + sizeof(vars_body_t::LANGUAGE), "eeprom body size does not match");

constexpr vars_body_t body_defaults = {
    old_eeprom::v6::body_defaults,
    5, // EEVAR_SOUND_VOLUME
    0xffff, // EEVAR_LANGUAGE
};

inline vars_body_t convert(const old_eeprom::v6::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v6 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v6::vars_body_t));

    return ret;
}

} // namespace config_store_ns::old_eeprom::v7
