/**
 * @file hw_configuration_common.cpp
 */

#include "hw_configuration_common.hpp"
#include "at21csxx_otp.hpp"
#include "otp.hpp"
#include <device/hal.h>

namespace buddy::hw {

#define USE_DUMMY_READ

#ifdef USE_DUMMY_READ
/**
 * @brief dummy read
 * because correct (commented )
 *
 * @return XlcdEeprom
 */
static XlcdEeprom read_xlcd() {

    OTP_v2 ret {};
    ret.version = 2;
    ret.size = sizeof(OTP_v2);
    ret.bomID = 12;

    return ret;
}
#else
/**
 * @brief use this  function only once during startup!!!
 * currently LoveBoardEeprom has to be OTP_v2
 *
 * @return LoveBoardEeprom data from loveboards eeprom
 */
static XlcdEeprom read_xlcd() {
    // LCD reset
    __HAL_RCC_GPIOG_CLK_ENABLE(); // enable lcd reset pin port clock
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    __HAL_RCC_GPIOC_CLK_ENABLE(); // enable lcd eeprom pin port clock
    OtpFromEeprom XlcdEeprom = OtpFromEeprom(GPIOC, GPIO_PIN_8);
    return XlcdEeprom.calib_data;
}
#endif

ConfigurationCommon::ConfigurationCommon()
    : xlcd_eeprom(read_xlcd()) {
    bom_id = otp_get_bom_id().value_or(0);
    bom_id_xlcd = otp_parse_bom_id(reinterpret_cast<uint8_t *>(&xlcd_eeprom), sizeof(xlcd_eeprom)).value_or(0);
}

} // namespace buddy::hw
