/**
 * @file hw_configuration.cpp
 */

#include "hw_configuration.hpp"
#include "bsod.h"
#include "otp.hpp"

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration();
    return ths;
}

} // namespace buddy::hw
