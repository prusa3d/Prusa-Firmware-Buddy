#pragma once

#include <cstdlib>
#include <device/hal.h>

class [[nodiscard]] SPIBaudRatePrescalerGuard {
private:
    SPI_HandleTypeDef *hspi;
    uint32_t old_prescaler;
    bool enabled;

public:
    SPIBaudRatePrescalerGuard(SPI_HandleTypeDef *hspi, uint32_t new_prescaler, bool enable = true)
        : hspi { hspi }
        , old_prescaler { hspi->Init.BaudRatePrescaler }
        , enabled { enable && old_prescaler < new_prescaler } {
        if (!enabled) {
            return;
        }

        if (HAL_SPI_DeInit(hspi) != HAL_OK) {
            abort();
        }
        hspi->Init.BaudRatePrescaler = new_prescaler;
        if (HAL_SPI_Init(hspi) != HAL_OK) {
            abort();
        }
    }

    ~SPIBaudRatePrescalerGuard() {
        if (!enabled) {
            return;
        }

        if (HAL_SPI_DeInit(hspi) != HAL_OK) {
            abort();
        }
        hspi->Init.BaudRatePrescaler = old_prescaler;
        if (HAL_SPI_Init(hspi) != HAL_OK) {
            abort();
        }
    }
};
