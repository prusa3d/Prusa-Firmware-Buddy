/**
 * @file MItem_eeprom.cpp
 * @author Radek Vana
 * @date 2021-09-22
 */
#include "MItem_eeprom.hpp"
#include "i2c.h"
#include "eeprom.h"
#include "eeprom_loadsave.h"
#include "GuiDefaults.hpp"
#include "i18n.h"

static constexpr bool use_long_text = GuiDefaults::infoDefaultLen >= 16;
// dev only, don't translate
constexpr static const char *label_TRANSMIT_OK = (use_long_text) ? "TRANSMIT_OK" : "T_OK";
constexpr static const char *label_TRANSMIT_ERROR = (use_long_text) ? "TRANSMIT_ERROR" : "T_ERROR";
constexpr static const char *label_TRANSMIT_BUSY = (use_long_text) ? "TRANSMIT_BUSY" : "T_BUSY";
constexpr static const char *label_TRANSMIT_TIMEOUT = (use_long_text) ? "TRANSMIT_TIMEOUT" : "R_TIMEOUT";
constexpr static const char *label_TRANSMIT_UNDEF = (use_long_text) ? "TRANSMIT_UNDEF" : "T_UNDEF";
constexpr static const char *label_RECEIVE_OK = (use_long_text) ? "RECEIVE_OK" : "R_OK";
constexpr static const char *label_RECEIVE_ERROR = (use_long_text) ? "RECEIVE_ERROR" : "R_ERROR";
constexpr static const char *label_RECEIVE_BUSY = (use_long_text) ? "RECEIVE_BUSY" : "R_BUSY";
constexpr static const char *label_RECEIVE_TIMEOUT = (use_long_text) ? "RECEIVE_TIMEOUT" : "R_TIMEOUT";
constexpr static const char *label_RECEIVE_UNDEF = (use_long_text) ? "RECEIVE_UNDEF" : "R_UNDEF";

// accessing extern variables from other thread
// uint32_t will be most likely atomic, if it is not it will not break anything because it is "read only"
// meant to be used in dev mode

MI_I2C_TRANSMIT_RESULTS_HAL_OK::MI_I2C_TRANSMIT_RESULTS_HAL_OK()
    : WI_INFO_DEV_t(I2C_TRANSMIT_RESULTS_HAL_OK, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_TRANSMIT_OK)) {
}

MI_I2C_TRANSMIT_RESULTS_HAL_ERROR::MI_I2C_TRANSMIT_RESULTS_HAL_ERROR()
    : WI_INFO_DEV_t(I2C_TRANSMIT_RESULTS_HAL_ERROR, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_TRANSMIT_ERROR)) {
}

MI_I2C_TRANSMIT_RESULTS_HAL_BUSY::MI_I2C_TRANSMIT_RESULTS_HAL_BUSY()
    : WI_INFO_DEV_t(I2C_TRANSMIT_RESULTS_HAL_BUSY, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_TRANSMIT_BUSY)) {
}

MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT::MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT()
    : WI_INFO_DEV_t(I2C_TRANSMIT_RESULTS_HAL_TIMEOUT, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_TRANSMIT_TIMEOUT)) {
}

MI_I2C_TRANSMIT_RESULTS_UNDEF::MI_I2C_TRANSMIT_RESULTS_UNDEF()
    : WI_INFO_DEV_t(I2C_TRANSMIT_RESULTS_UNDEF, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_TRANSMIT_UNDEF)) {
}

MI_I2C_RECEIVE_RESULTS_HAL_OK::MI_I2C_RECEIVE_RESULTS_HAL_OK()
    : WI_INFO_DEV_t(I2C_RECEIVE_RESULTS_HAL_OK, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_RECEIVE_OK)) {
}

MI_I2C_RECEIVE_RESULTS_HAL_ERROR::MI_I2C_RECEIVE_RESULTS_HAL_ERROR()
    : WI_INFO_DEV_t(I2C_RECEIVE_RESULTS_HAL_ERROR, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_RECEIVE_ERROR)) {
}

MI_I2C_RECEIVE_RESULTS_HAL_BUSY::MI_I2C_RECEIVE_RESULTS_HAL_BUSY()
    : WI_INFO_DEV_t(I2C_RECEIVE_RESULTS_HAL_BUSY, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_RECEIVE_BUSY)) {
}

MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT::MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT()
    : WI_INFO_DEV_t(I2C_RECEIVE_RESULTS_HAL_TIMEOUT, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_RECEIVE_TIMEOUT)) {
}

MI_I2C_RECEIVE_RESULTS_UNDEF::MI_I2C_RECEIVE_RESULTS_UNDEF()
    : WI_INFO_DEV_t(I2C_RECEIVE_RESULTS_UNDEF, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_RECEIVE_UNDEF)) {
}
