#pragma once

#include <stm32g0xx_hal.h>

// Pin assignment as designed in htcontroller.ioc

#define D_RS_RESET_GPIO_Port GPIOA
#define D_RS_RESET_Pin       GPIO_PIN_10

#define RS485_TX_GPIO_Port GPIOC
#define RS485_TX_Pin       GPIO_PIN_4
#define RS485_RX_GPIO_Port GPIOC
#define RS485_RX_Pin       GPIO_PIN_5
#define RS485_DE_GPIO_Port GPIOA
#define RS485_DE_Pin       GPIO_PIN_12

#define A_NTC_AMBIENT_GPIO_Port GPIOA
#define A_NTC_AMBIENT_Pin       GPIO_PIN_4 // ADC1_IN4
#define A_NTC_HEATER_GPIO_Port  GPIOA
#define A_NTC_HEATER_Pin        GPIO_PIN_5 // ADC1_IN5
#define A_NTC_CHAMBER_GPIO_Port GPIOB
#define A_NTC_CHAMBER_Pin       GPIO_PIN_2 // ADC1_IN10

#define A_HEATER_CURRENT_GPIO_Port GPIOB
#define A_HEATER_CURRENT_Pin       GPIO_PIN_0 // ADC1_IN8

#define D_ENABLE_GPIO_Port GPIOB
#define D_ENABLE_Pin       GPIO_PIN_1

#define D_FAULT_GPIO_Port GPIOA
#define D_FAULT_Pin       GPIO_PIN_8 // EXTI8

#define D_HEATER_PWM_GPIO_Port GPIOB
#define D_HEATER_PWM_Pin       GPIO_PIN_3 // TIM1_CH2

#define FAN_HEATER_0_PWM_GPIO_Port GPIOC
#define FAN_HEATER_0_PWM_Pin       GPIO_PIN_6 // TIM3_CH1
#define FAN_HEATER_1_PWM_GPIO_Port GPIOC
#define FAN_HEATER_1_PWM_Pin       GPIO_PIN_7 // TIM3_CH2

#define FAN_HEATER_0_TACH_GPIO_Port GPIOC
#define FAN_HEATER_0_TACH_Pin       GPIO_PIN_2 // TIM15_CH2
#define FAN_HEATER_1_TACH_GPIO_Port GPIOC
#define FAN_HEATER_1_TACH_Pin       GPIO_PIN_1 // TIM15_CH1

#define FAN_HEATER_0_ENABLE_GPIO_Port GPIOD
#define FAN_HEATER_0_ENABLE_Pin       GPIO_PIN_8
#define FAN_HEATER_1_ENABLE_GPIO_Port GPIOD
#define FAN_HEATER_1_ENABLE_Pin       GPIO_PIN_9

namespace hal::peripherals {
/// HAL tick and microsecond time base, see timing.h
extern TIM_HandleTypeDef htim7;
} // namespace hal::peripherals
