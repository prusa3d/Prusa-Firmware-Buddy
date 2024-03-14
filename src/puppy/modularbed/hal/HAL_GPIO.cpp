#include "hal/HAL_GPIO.hpp"
#include "hal/HAL_System.hpp"
#include "stm32g0xx_hal.h"
#include <device/board.h>

namespace hal::GPIODriver {

bool Init() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct {};

    // Reset-OvercurrentFault
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PANIC + FAULT signals
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    ResetOverCurrentFault();

    return true;
}

bool ReadPANICSignal() {
    bool value = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11) == GPIO_PIN_RESET ? true : false;
    return value;
}

bool ReadFAULTSignal() {
    bool value = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12) == GPIO_PIN_RESET ? true : false;
    return value;
}

void ResetOverCurrentFault() {
// ACS711 datasheet requires that Vcc shall be < 200mV for at least 100 microseconds
#if (BOARD_VER_EQUAL_TO(0, 6, 0))
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    hal::System::WaitMicroseconds(500);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    hal::System::WaitMicroseconds(500);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
#endif
}

} // namespace hal::GPIODriver
