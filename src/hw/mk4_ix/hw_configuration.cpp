/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "otp.hpp"
#include "timing_precise.hpp"
#include <option/bootloader.h>

#if BOOTLOADER()
    #include "data_exchange.hpp"
static std::pair<LoveBoardEeprom, OtpStatus> read_loveboard() {
    return { data_exchange::get_loveboard_eeprom(), data_exchange::get_loveboard_status() };
}
#else
    #include "at21csxx_otp.hpp"
    #include <device/hal.h>

using buddy::hw::hx717Dout;
using buddy::hw::hx717Sck;
using buddy::hw::Pin;

/**
 * @brief use this  function only once during startup!!!
 * currently LoveBoardEeprom has to be OTP_v2
 *
 * @return LoveBoardEeprom data from loveboards eeprom
 */
static std::pair<LoveBoardEeprom, OtpStatus> read_loveboard() {
    __HAL_RCC_GPIOF_CLK_ENABLE(); // enable loveboard eeprom pin port clock
    OtpFromEeprom LoveBoard = OtpFromEeprom(GPIOF, GPIO_PIN_13);
    return { LoveBoard.calib_data, LoveBoard.get_status() };
}
#endif

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration(read_loveboard());
    return ths;
}

Configuration::Configuration(std::pair<LoveBoardEeprom, OtpStatus> loveboard_)
    : loveboard(loveboard_) {
}

float Configuration::curr_measurement_voltage_to_current(float voltage) const {
    constexpr float allegro_curr_from_voltage = 1 / 0.09F;

    const float allegro_zero_curr_voltage = (get_board_bom_id() == 27) ? 5.F / 2.F : 3.35F / 2.F; // choose half of 3V3 or 5V range

    return (voltage - allegro_zero_curr_voltage) * allegro_curr_from_voltage;
}

bool Configuration::is_fw_incompatible_with_hw() {
#if PRINTER_IS_PRUSA_MK4
    if (get_loveboard_status().data_valid) {
        return false; // valid data, fw compatible
    }

    // This procedure is checking if MK3.5 extruder is installed,
    // in which case we have an incompatible FW (MK4) and HW (MK3.5)
    // It is not possible to continue and info screen saying to reflash has to pop up
    const size_t count_of_validation_edges = 4;
    bool mk35_extruder_detected = true;
    for (size_t i = 0; i < count_of_validation_edges; ++i) {
        hx717Sck.write(Pin::State::low);
        delay_us_precise<1000>();
        if (hx717Dout.read() != Pin::State::low) {
            mk35_extruder_detected = false;
            break;
        }
        hx717Sck.write(Pin::State::high);
        delay_us_precise<1000>();
        if (hx717Dout.read() != Pin::State::high) {
            mk35_extruder_detected = false;
            break;
        }
    }

    return mk35_extruder_detected;

#else
    return false; // There is no need for this compatibility check on other build configurations
#endif
}

} // namespace buddy::hw
