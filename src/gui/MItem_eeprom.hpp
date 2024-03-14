/**
 * @file MItem_eeprom.hpp
 * @author Radek Vana
 * @brief Menu items to display eeprom status
 * @date 2021-09-22
 */
#pragma once
#include "WindowMenuInfo.hpp"
#include "i2c.hpp"
class MI_EEPROM_INIT_CRC_ERROR : public WI_INFO_DEV_t {
public:
    MI_EEPROM_INIT_CRC_ERROR();
};

class MI_EEPROM_INIT_UPGRADED : public WI_INFO_DEV_t {
public:
    MI_EEPROM_INIT_UPGRADED();
};

class MI_EEPROM_INIT_UPGRADE_FAILED : public WI_INFO_DEV_t {
public:
    MI_EEPROM_INIT_UPGRADE_FAILED();
};

const uint8_t *get_label_ok(uint8_t channel);
const uint8_t *get_label_error(uint8_t channel);
const uint8_t *get_label_busy(uint8_t channel);
const uint8_t *get_label_timeout(uint8_t channel);

template <uint8_t CHAN>
class MI_I2C_RESULTS_HAL_OK : public WI_INFO_DEV_t {
public:
    MI_I2C_RESULTS_HAL_OK()
        : WI_INFO_DEV_t(i2c::statistics::get_hal_ok(CHAN), string_view_utf8::MakeCPUFLASH(get_label_ok(CHAN))) {
    }
};

template <uint8_t CHAN>
class MI_I2C_RESULTS_HAL_ERROR : public WI_INFO_DEV_t {
public:
    MI_I2C_RESULTS_HAL_ERROR()
        : WI_INFO_DEV_t(i2c::statistics::get_hal_error(CHAN), string_view_utf8::MakeCPUFLASH(get_label_error(CHAN))) {
    }
};

template <uint8_t CHAN>
class MI_I2C_RESULTS_HAL_BUSY : public WI_INFO_DEV_t {
public:
    MI_I2C_RESULTS_HAL_BUSY()
        : WI_INFO_DEV_t(i2c::statistics::get_hal_busy(CHAN), string_view_utf8::MakeCPUFLASH(get_label_busy(CHAN))) {
    }
};

template <uint8_t CHAN>
class MI_I2C_RESULTS_HAL_TIMEOUT : public WI_INFO_DEV_t {
public:
    MI_I2C_RESULTS_HAL_TIMEOUT()
        : WI_INFO_DEV_t(i2c::statistics::get_hal_timeout(CHAN), string_view_utf8::MakeCPUFLASH(get_label_timeout(CHAN))) {
    }
};
