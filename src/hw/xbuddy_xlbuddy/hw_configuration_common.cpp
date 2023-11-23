/**
 * @file hw_configuration_common.cpp
 */

#include "hw_configuration_common.hpp"
#include "at21csxx_otp.hpp"
#include "otp.hpp"
#include <device/hal.h>
#include <option/bootloader.h>

#if BOOTLOADER()
    #include "data_exchange.hpp"
static std::pair<XlcdEeprom, OtpStatus> read_xlcd() {
    return { data_exchange::get_xlcd_eeprom(), data_exchange::get_xlcd_status() };
}
#else
/**
 * @brief use this  function only once during startup!!!
 * currently LoveBoardEeprom has to be OTP_v2
 *
 * @return LoveBoardEeprom data from loveboards eeprom + error counts
 */
static std::pair<XlcdEeprom, OtpStatus> read_xlcd() {
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
    return { XlcdEeprom.calib_data, XlcdEeprom.get_status() };
}
#endif
namespace buddy::hw {

ConfigurationCommon::ConfigurationCommon()
    : xlcd(read_xlcd()) {
    bom_id = otp_get_bom_id().value_or(0);
    bom_id_xlcd = otp_parse_bom_id(reinterpret_cast<uint8_t *>(&std::get<XlcdEeprom>(xlcd)), sizeof(XlcdEeprom)).value_or(0);
}

} // namespace buddy::hw
