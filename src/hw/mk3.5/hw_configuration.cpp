/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "bsod.h"
#include "otp.hpp"
#include <option/bootloader.h>

#if BOOTLOADER()
    #include "data_exchange.hpp"
static bool loveboard_detected() {
    return data_exchange::get_loveboard_status().data_valid;
}
#else
    #include "at21csxx_otp.hpp"
    #include <device/hal.h>

/**
 * @brief use this  function only once during startup!!!
 * it must return false since mk3.5 does not have loveboard
 *
 * @return if loveboard was detected
 */
static bool loveboard_detected() {
    __HAL_RCC_GPIOF_CLK_ENABLE(); // enable loveboard eeprom pin port clock
    OtpFromEeprom LoveBoard = OtpFromEeprom(GPIOF, GPIO_PIN_13);
    return LoveBoard.get_status().data_valid;
}
#endif

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration();
    return ths;
}

Configuration::Configuration() {
    auto bom_id = otp_get_bom_id();

    if (!bom_id || *bom_id == 27) {
        bsod("Wrong board version");
    }

    error__loveboard_detected = loveboard_detected();
}

/**
 * valid data from loveboard mean, tht we have MK4 HW, since MK3.5 does not have loveboard
 */
bool Configuration::is_fw_incompatible_with_hw() {
    return error__loveboard_detected;
}

} // namespace buddy::hw
