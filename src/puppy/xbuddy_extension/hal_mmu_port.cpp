/// @file
#include "hal_mmu_port.hpp"

#include <stm32h5xx_hal.h>

#if HAS_GPIO_EXPANDER()
    #include "hal_gpio_expander.hpp"
#endif

static void nreset_pin_init() {
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void hal::mmu_port::init() {
    nreset_pin_init();
    nreset_pin_set(false);
}

#if HAS_MMU_POWER_PIN()
void hal::mmu_port::power_pin_set(bool b) {
    hal::gpio_expander::write(hal::gpio_expander::Pin::mmu_power, b);
}
bool hal::mmu_port::power_pin_get() {
    return hal::gpio_expander::read(hal::gpio_expander::Pin::mmu_power);
}
#endif

bool hal::mmu_port::nreset_pin_get() {
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET;
}

void hal::mmu_port::nreset_pin_set(bool b) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
