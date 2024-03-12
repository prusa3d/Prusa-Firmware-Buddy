/**
 * @file hw_configuration_common.cpp
 */

#include "hw_configuration_common.hpp"
#include "data_exchange.hpp"
#include "otp.hpp"
#include <device/hal.h>
#include <option/bootloader.h>

static std::pair<XlcdEeprom, OtpStatus> read_xlcd() {
    return { data_exchange::get_xlcd_eeprom(), data_exchange::get_xlcd_status() };
}

namespace buddy::hw {

ConfigurationCommon::ConfigurationCommon()
    : xlcd(read_xlcd()) {
    bom_id = otp_get_bom_id().value_or(0);
    bom_id_xlcd = otp_parse_bom_id(reinterpret_cast<uint8_t *>(&std::get<XlcdEeprom>(xlcd)), sizeof(XlcdEeprom)).value_or(0);
}

} // namespace buddy::hw
