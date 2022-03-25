/**
 * @file eeprom_v10.hpp
 * @author Radek Vana
 * @brief old version of eeprom, to be able to import it
 * version 10 from release 4.3.3-RC
 * without padding and crc since they are not imported and would not match anyway
 * @date 2022-01-17
 */

#include "eeprom_v9.hpp"

namespace eeprom::v10 {

#pragma once
#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v10
 * without head, padding and crc
 */
struct vars_body_t : public eeprom::v9::vars_body_t {
    uint32_t FOOTER_SETTING;
    uint32_t FOOTER_DRAW_TYPE;
    uint8_t FAN_CHECK_ENABLED;
    uint8_t FS_AUTOLOAD_ENABLED;
    float EEVAR_ODOMETER_X;
    float EEVAR_ODOMETER_Y;
    float EEVAR_ODOMETER_Z;
    float EEVAR_ODOMETER_E0;
    float AXIS_STEPS_PER_UNIT_X;
    float AXIS_STEPS_PER_UNIT_Y;
    float AXIS_STEPS_PER_UNIT_Z;
    float AXIS_STEPS_PER_UNIT_E0;
    uint16_t AXIS_MICROSTEPS_X;
    uint16_t AXIS_MICROSTEPS_Y;
    uint16_t AXIS_MICROSTEPS_Z;
    uint16_t AXIS_MICROSTEPS_E0;
    uint16_t AXIS_RMS_CURRENT_MA_X;
    uint16_t AXIS_RMS_CURRENT_MA_Y;
    uint16_t AXIS_RMS_CURRENT_MA_Z;
    uint16_t AXIS_RMS_CURRENT_MA_E0;
    float AXIS_Z_MAX_POS_MM;
    uint32_t ODOMETER_TIME;
};

#pragma pack(pop)

static_assert(sizeof(vars_body_t) == sizeof(eeprom::v9::vars_body_t) + sizeof(uint32_t) * 3 + sizeof(uint8_t) * 2 + sizeof(float) * 9 + sizeof(uint16_t) * 8, "eeprom body size does not match");

static constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;

constexpr vars_body_t body_defaults = {
    eeprom::v9::body_defaults,
    footer::eeprom::Encode(footer::DefaultItems),                               // EEVAR_FOOTER_SETTING
    uint32_t(footer::ItemDrawCnf::Default()),                                   // EEVAR_FOOTER_DRAW_TYPE
    true,                                                                       // EEVAR_FAN_CHECK_ENABLED
    true,                                                                       // EEVAR_FS_AUTOLOAD_ENABLED
    0,                                                                          // EEVAR_ODOMETER_X
    0,                                                                          // EEVAR_ODOMETER_Y
    0,                                                                          // EEVAR_ODOMETER_Z
    0,                                                                          // EEVAR_ODOMETER_E0
    default_axis_steps_flt[0] * ((DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_X
    default_axis_steps_flt[1] * ((DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Y
    default_axis_steps_flt[2] * ((DEFAULT_INVERT_Z_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Z
    default_axis_steps_flt[3] * ((DEFAULT_INVERT_E0_DIR == true) ? -1.f : 1.f), // AXIS_STEPS_PER_UNIT_E0
    X_MICROSTEPS,                                                               // AXIS_MICROSTEPS_X
    Y_MICROSTEPS,                                                               // AXIS_MICROSTEPS_Y
    Z_MICROSTEPS,                                                               // AXIS_MICROSTEPS_Z
    E0_MICROSTEPS,                                                              // AXIS_MICROSTEPS_E0
    X_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_X
    Y_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_Y
    Z_CURRENT,                                                                  // AXIS_RMS_CURRENT_MA_Z
    E0_CURRENT,                                                                 // AXIS_RMS_CURRENT_MA_E0
#ifdef DEFAULT_Z_MAX_POS
    DEFAULT_Z_MAX_POS, // AXIS_Z_MAX_POS_MM
#else
    0,
#endif
    0, // EEVAR_ODOMETER_TIME
};

inline vars_body_t convert(const eeprom::v9::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v9 struct
    memcpy(&ret, &src, sizeof(eeprom::v9::vars_body_t));

    return ret;
}

} // namespace
