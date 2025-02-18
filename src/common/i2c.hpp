#pragma once

#include "inttypes.h"
#include <device/peripherals.h>
#include "cmsis_os.h"

namespace i2c {

// numbered because of touch read return value
enum class Result {
    ok = 0,
    error = 1,
    busy_after_retries = 2,
    timeout = 3
};

class ChannelMutex {
public:
    /// Initializes the mutexes. Need to be called
    static void static_init();

    [[nodiscard]] ChannelMutex(I2C_HandleTypeDef &hi2c);
    ~ChannelMutex();

    static osMutexId get_handle(I2C_HandleTypeDef &hi2c);

private:
    // osMutexId is void*, dont use osMutexId in header - lower dependency
    osMutexId mutex_handle;
};

[[nodiscard]] Result Transmit(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
[[nodiscard]] Result Receive(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

[[nodiscard]] Result Mem_Write_8bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
[[nodiscard]] Result Mem_Read_8bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
[[nodiscard]] Result Mem_Write_16bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
[[nodiscard]] Result Mem_Read_16bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

[[nodiscard]] Result IsDeviceReady(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);

} // namespace i2c
