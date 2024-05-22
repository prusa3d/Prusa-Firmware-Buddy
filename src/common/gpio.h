// gpio.h
#pragma once

#include <device/hal.h>

// port of pin8
static inline GPIO_TypeDef *gpio_port(uint8_t pin8) {
    return (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * (pin8 >> 4));
}

static inline int gpio_get(uint8_t pin8) {
    return (HAL_GPIO_ReadPin(gpio_port(pin8), (1 << (pin8 & 0x0f))) != GPIO_PIN_RESET) ? 1 : 0;
}

static inline void gpio_set(uint8_t pin8, int state) {
    HAL_GPIO_WritePin(gpio_port(pin8), (1 << (pin8 & 0x0f)), state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void gpio_init(uint8_t pin8, uint32_t mode, uint32_t pull, uint32_t speed) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = (1 << (pin8 & 0x0f));
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = speed;
    HAL_GPIO_Init(gpio_port(pin8), &GPIO_InitStruct);
}
