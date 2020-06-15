// gpio.h
#ifndef _GPIO_H
#define _GPIO_H

#include "stm32f4xx_hal.h"

//pin definitions (PA-PE)
#define TPA0  0
#define TPA1  1
#define TPA2  2
#define TPA3  3
#define TPA4  4
#define TPA5  5
#define TPA6  6
#define TPA7  7
#define TPA8  8
#define TPA9  9
#define TPA10 10
#define TPA11 11
#define TPA12 12
#define TPA13 13
#define TPA14 14
#define TPA15 15
#define TPB0  16
#define TPB1  17
#define TPB2  18
#define TPB3  19
#define TPB4  20
#define TPB5  21
#define TPB6  22
#define TPB7  23
#define TPB8  24
#define TPB9  25
#define TPB10 26
#define TPB11 27
#define TPB12 28
#define TPB13 29
#define TPB14 30
#define TPB15 31
#define TPC0  32
#define TPC1  33
#define TPC2  34
#define TPC3  35
#define TPC4  36
#define TPC5  37
#define TPC6  38
#define TPC7  39
#define TPC8  40
#define TPC9  41
#define TPC10 42
#define TPC11 43
#define TPC12 44
#define TPC13 45
#define TPC14 46
#define TPC15 47
#define TPD0  48
#define TPD1  49
#define TPD2  50
#define TPD3  51
#define TPD4  52
#define TPD5  53
#define TPD6  54
#define TPD7  55
#define TPD8  56
#define TPD9  57
#define TPD10 58
#define TPD11 59
#define TPD12 60
#define TPD13 61
#define TPD14 62
#define TPD15 63
#define TPE0  64
#define TPE1  65
#define TPE2  66
#define TPE3  67
#define TPE4  68
#define TPE5  69
#define TPE6  70
#define TPE7  71
#define TPE8  72
#define TPE9  73
#define TPE10 74
#define TPE11 75
#define TPE12 76
#define TPE13 77
#define TPE14 78
#define TPE15 79

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
