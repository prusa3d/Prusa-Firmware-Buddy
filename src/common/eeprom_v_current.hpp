/**
 * @file eeprom_current.hpp
 * @brief current version of eeprom
 */

#include "eeprom_v32787.hpp"
namespace eeprom::current {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of current eeprom
 * without head and crc
 */
struct vars_body_t : public eeprom::v32787::vars_body_t {
    float HOMING_BDIVISOR_X;
    float HOMING_BDIVISOR_Y;
    bool EEVAR_ENABLE_SIDE_LEDS;
};

#pragma pack(pop)

constexpr vars_body_t body_defaults = {
    eeprom::v32787::body_defaults,
    0.0f, // homing bump divisor
    0.0f, // homing bump divisor
    true, // EEVAR_ENABLE_SIDE_LEDS
};

inline vars_body_t convert(const eeprom::v32787::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v32787 struct
    memcpy(&ret, &src, sizeof(eeprom::v32787::vars_body_t));

    return ret;
}

} // namespace
