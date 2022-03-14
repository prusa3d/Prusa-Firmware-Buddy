/**
 * @file MItem_eeprom.hpp
 * @author Radek Vana
 * @brief Menu items to display eeprom status
 * @date 2021-09-22
 */
#pragma once
#include "WindowMenuInfo.hpp"

class MI_I2C_TRANSMIT_RESULTS_HAL_OK : public WI_INFO_DEV_t {
public:
    MI_I2C_TRANSMIT_RESULTS_HAL_OK();
};

class MI_I2C_TRANSMIT_RESULTS_HAL_ERROR : public WI_INFO_DEV_t {
public:
    MI_I2C_TRANSMIT_RESULTS_HAL_ERROR();
};

class MI_I2C_TRANSMIT_RESULTS_HAL_BUSY : public WI_INFO_DEV_t {
public:
    MI_I2C_TRANSMIT_RESULTS_HAL_BUSY();
};

class MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT : public WI_INFO_DEV_t {
public:
    MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT();
};

class MI_I2C_TRANSMIT_RESULTS_UNDEF : public WI_INFO_DEV_t {
public:
    MI_I2C_TRANSMIT_RESULTS_UNDEF();
};

class MI_I2C_RECEIVE_RESULTS_HAL_OK : public WI_INFO_DEV_t {
public:
    MI_I2C_RECEIVE_RESULTS_HAL_OK();
};

class MI_I2C_RECEIVE_RESULTS_HAL_ERROR : public WI_INFO_DEV_t {
public:
    MI_I2C_RECEIVE_RESULTS_HAL_ERROR();
};

class MI_I2C_RECEIVE_RESULTS_HAL_BUSY : public WI_INFO_DEV_t {
public:
    MI_I2C_RECEIVE_RESULTS_HAL_BUSY();
};

class MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT : public WI_INFO_DEV_t {
public:
    MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT();
};

class MI_I2C_RECEIVE_RESULTS_UNDEF : public WI_INFO_DEV_t {
public:
    MI_I2C_RECEIVE_RESULTS_UNDEF();
};
