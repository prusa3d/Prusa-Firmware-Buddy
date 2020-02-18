// gpio.h
#ifndef _GPIO_H
#define _GPIO_H

#include "stm32f4xx_hal.h"

//pin definitions (PA-PE)
#define PA0  0
#define PA1  1
#define PA2  2
#define PA3  3
#define PA4  4
#define PA5  5
#define PA6  6
#define PA7  7
#define PA8  8
#define PA9  9
#define PA10 10
#define PA11 11
#define PA12 12
#define PA13 13
#define PA14 14
#define PA15 15
#define PB0  16
#define PB1  17
#define PB2  18
#define PB3  19
#define PB4  20
#define PB5  21
#define PB6  22
#define PB7  23
#define PB8  24
#define PB9  25
#define PB10 26
#define PB11 27
#define PB12 28
#define PB13 29
#define PB14 30
#define PB15 31
#define PC0  32
#define PC1  33
#define PC2  34
#define PC3  35
#define PC4  36
#define PC5  37
#define PC6  38
#define PC7  39
#define PC8  40
#define PC9  41
#define PC10 42
#define PC11 43
#define PC12 44
#define PC13 45
#define PC14 46
#define PC15 47
#define PD0  48
#define PD1  49
#define PD2  50
#define PD3  51
#define PD4  52
#define PD5  53
#define PD6  54
#define PD7  55
#define PD8  56
#define PD9  57
#define PD10 58
#define PD11 59
#define PD12 60
#define PD13 61
#define PD14 62
#define PD15 63
#define PE0  64
#define PE1  65
#define PE2  66
#define PE3  67
#define PE4  68
#define PE5  69
#define PE6  70
#define PE7  71
#define PE8  72
#define PE9  73
#define PE10 74
#define PE11 75
#define PE12 76
#define PE13 77
#define PE14 78
#define PE15 79

//port of pin8
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
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = (1 << (pin8 & 0x0f));
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = speed;
    HAL_GPIO_Init(gpio_port(pin8), &GPIO_InitStruct);
}

#endif //_GPIO_H
