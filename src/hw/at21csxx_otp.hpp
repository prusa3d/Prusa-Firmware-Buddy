
#pragma once

#include "otp_types.hpp"
#include "../hw/at21csxx.hpp"

/**
 * @brief reads OTP data from external onewire AT21CSxx eeprom
 * currently supports only OTP_v2 (and OTP_v5 since it is the same)
 * supporting obsolete OTP_v0, OTP_v1 and OTP_v3 does not make sense
 * and OTP_v4 is the same as OTP_v2/OTP_v5 with extra mac address
 */
class OtpFromEeprom {
public:
    OtpFromEeprom(GPIO_TypeDef *port, uint32_t pin);

    bool loadData();

    OTP_v2 calib_data;

    OtpStatus get_status() { return status; }

private:
    AT21CSxx eeprom;
    OtpStatus status;
};
