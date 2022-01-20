/**
 * @file eeprom_v7.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 7 from release 4.2.0-RC1
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v6.hpp"

namespace eeprom::v7 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body od eeprom v7
 * without head, padding and crc
 */
struct vars_body_t : public eeprom::v6::vars_body_t {
    constexpr vars_body_t()
        : SOUND_VOLUME(5)
        , LANGUAGE(0xffff) {}
    uint8_t SOUND_VOLUME;
    uint16_t LANGUAGE;
};
#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(eeprom::v6::vars_body_t) + sizeof(vars_body_t::SOUND_VOLUME) + sizeof(vars_body_t::LANGUAGE), "eeprom body size does not match");

static vars_body_t convert(const eeprom::v6::vars_body_t &src) {
    vars_body_t ret = vars_body_t();

    // copy entire v6 struct
    memcpy(&ret, &src, sizeof(eeprom::v6::vars_body_t));

    return ret;
}

} // namespace
