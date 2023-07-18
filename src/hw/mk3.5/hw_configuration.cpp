/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "bsod.h"
#include "otp.h"

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration();
    return ths;
}

Configuration::Configuration() {
    uint8_t bom_id;
    otp_get_bom_id(&bom_id);

    if (bom_id == 27) {
        bsod("Wrong board version");
    }
}

}
