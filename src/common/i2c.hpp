#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"

#include <atomic>

namespace i2c {
namespace statistics {
    uint32_t get_transmit_HAL_OK();
    uint32_t get_transmit_HAL_BUSY();
    uint32_t get_transmit_HAL_ERROR();
    uint32_t get_transmit_HAL_TIMEOUT();
    uint32_t get_receive_HAL_OK();
    uint32_t get_receive_HAL_BUSY();
    uint32_t get_receive_HAL_ERROR();
    uint32_t get_receive_HAL_TIMEOUT();
} // namespace statistics

enum class Result {
    ok,
    busy_after_retries,
    timeout,
    error
};

/**
 * @brief Transmit data on I2C, in case of error, throw redscreen
 */
[[nodiscard]] Result Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
 * @brief Transmit data on I2C, in case of error, just return it
 */
[[nodiscard]] HAL_StatusTypeDef Transmit_ext(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

[[nodiscard]] Result Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

}
