#pragma once
#include "stm32f4xx_hal.h"
#include <device/peripherals.h>

extern I2C_HandleTypeDef hi2c1;

namespace buddy {
namespace hw {
    class FUSB303 {
    public:
        static bool DetectChip() {
            HAL_StatusTypeDef status;
            status = HAL_I2C_IsDeviceReady(&I2C_HANDLE_FOR(eeprom), (uint16_t)(0x21 << 1), 2, 2);
            return status == HAL_OK;
        };
    };
}
}
