#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"

extern volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_OK;
extern volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_ERROR;
extern volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_BUSY;
extern volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_TIMEOUT;
extern volatile uint32_t I2C_TRANSMIT_RESULTS_UNDEF;

extern volatile uint32_t I2C_RECEIVE_RESULTS_HAL_OK;
extern volatile uint32_t I2C_RECEIVE_RESULTS_HAL_ERROR;
extern volatile uint32_t I2C_RECEIVE_RESULTS_HAL_BUSY;
extern volatile uint32_t I2C_RECEIVE_RESULTS_HAL_TIMEOUT;
extern volatile uint32_t I2C_RECEIVE_RESULTS_UNDEF;

extern void I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

extern void I2C_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
