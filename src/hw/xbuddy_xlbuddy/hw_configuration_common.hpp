/**
 * @file hw_configuration_common.hpp
 */

#pragma once
#include "otp_types.hpp"

namespace buddy::hw {
/**
 * @brief common hw configuration for xbuddy and xlbuddy boards
 * currently used only for XLCD
 */
class ConfigurationCommon {
    ConfigurationCommon(const ConfigurationCommon &) = delete;

    XlcdEeprom xlcd_eeprom;

    uint8_t bom_id { 0 };
    uint8_t bom_id_xlcd { 0 };

public:
    ConfigurationCommon();

    bool has_inverted_touch_interrupt() const { return bom_id_xlcd >= 28; }

    bool has_display_backlight_control() const { return bom_id_xlcd >= 29; }

    const LoveBoardEeprom &get_xlcd() const { return xlcd_eeprom; }

    uint8_t get_board_bom_id() const { return bom_id; }
};

} // namespace buddy::hw
