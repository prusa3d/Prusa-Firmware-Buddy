/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "data_exchange.hpp"
#include "bsod.h"
#include "otp.hpp"
#include <option/bootloader.h>

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

    error__loveboard_detected = data_exchange::get_loveboard_status().data_valid;
}

/**
 * valid data from loveboard mean, tht we have MK4 HW, since MK3.5 does not have loveboard
 */
bool Configuration::is_fw_incompatible_with_hw() {
    return error__loveboard_detected;
}

} // namespace buddy::hw
