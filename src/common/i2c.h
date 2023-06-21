#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
    #include <atomic>

extern volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_OK;
extern volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_BUSY;

extern volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_OK;
extern volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_BUSY;

extern "C" {
#endif // __cplusplus

/**
 * @brief Transmit data on I2C, in case of error, throw redscreen
 */
extern void I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
 * @brief Transmit data on I2C, in case of error, just return it
 */
extern HAL_StatusTypeDef I2C_Transmit_ext(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

extern void I2C_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#ifdef __cplusplus
}
#endif // __cplusplus
