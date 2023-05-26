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
    board_revision_t rev;
    otp_get_board_revision(&rev);

    if (rev.bytes[0] == 27) {
        bsod("Wrong board version");
    }
}

}
