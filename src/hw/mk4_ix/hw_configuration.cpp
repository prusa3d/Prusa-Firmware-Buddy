/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "calibrated_loveboard.hpp"
#include "otp.h"

/**
 * @brief use this  function only once during startup!!!
 *
 * @return LoveBoardEeprom data from loveboards eeprom
 */
static LoveBoardEeprom read_loveboard() {
    CalibratedLoveboard LoveBoard = CalibratedLoveboard(GPIOF, LL_GPIO_PIN_13);
    return LoveBoard.calib_data;
}

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration(read_loveboard());
    return ths;
}

Configuration::Configuration(const LoveBoardEeprom &eeprom) {
    loveboard_eeprom = eeprom;

    otp_get_bom_id(&bom_id);
}

float Configuration::curr_measurement_voltage_to_current(float voltage) const {
    constexpr float allegro_curr_from_voltage = 1 / 0.09F;

    const float allegro_zero_curr_voltage = (bom_id == 27) ? 5.F / 2.F : 3.35F / 2.F; // choose half of 3V3 or 5V range

    return (voltage - allegro_zero_curr_voltage) * allegro_curr_from_voltage;
}

}
