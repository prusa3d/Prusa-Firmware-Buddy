/**
 * Marlin 3D Printer Firmware
 *
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 * Copyright (c) 2016 Bob Cousins bobcousins42@googlemail.com
 * Copyright (c) 2015-2016 Nico Tonnhofer wurstnase.reprap@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if defined(STM32GENERIC) && defined(STM32G0)

#include "../HAL.h"
#include "timers.h"
#include "buddy/priorities_config.h"

// ------------------------
// Local defines
// ------------------------

#define NUM_HARDWARE_TIMERS 3
#define STEP_TIMER_IRQ_ID TIM1_CC_IRQn
#define MOVE_TIMER_IRQ_ID TIM6_IRQn
#define TEMP_TIMER_IRQ_ID TIM7_IRQn

// ------------------------
// Private Variables
// ------------------------

stm32_timer_t TimerHandle[NUM_HARDWARE_TIMERS];

// ------------------------
// Public functions
// ------------------------

bool timers_initialized[NUM_HARDWARE_TIMERS] = {false};

void HAL_timer_start(const uint8_t timer_num, const uint32_t frequency) {
  if(timer_num >= NUM_HARDWARE_TIMERS) {
    abort();
  }

  if (!timers_initialized[timer_num]) {
    constexpr uint32_t step_prescaler = STEPPER_TIMER_PRESCALE - 1,
                       move_prescaler = MOVE_TIMER_PRESCALE - 1,
                       temp_prescaler = TEMP_TIMER_PRESCALE - 1;
    switch (timer_num) {
      case STEP_TIMER_NUM:
        __HAL_RCC_TIM1_CLK_ENABLE();
        TimerHandle[timer_num].handle.Instance               = TIM1;
        TimerHandle[timer_num].handle.Init.Period            = 65535;
        TimerHandle[timer_num].handle.Init.Prescaler         = step_prescaler; // Frequency of timer ticks should be 1MHz, based on 168000000 / (167 + 1).
        TimerHandle[timer_num].handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
        TimerHandle[timer_num].handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
        TimerHandle[timer_num].handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        TimerHandle[timer_num].callback = (uint32_t)TC1_Handler;
        HAL_NVIC_SetPriority(STEP_TIMER_IRQ_ID,  ISR_PRIORITY_STEP_TIMER, 0);
        break;

      case MOVE_TIMER_NUM:
        // MOVE TIMER TIM6 - any available 16bit Timer
        __HAL_RCC_TIM6_CLK_ENABLE();
        TimerHandle[timer_num].handle.Instance            = TIM6;
        TimerHandle[timer_num].handle.Init.Prescaler      = move_prescaler;
        TimerHandle[timer_num].handle.Init.CounterMode    = TIM_COUNTERMODE_UP;
        TimerHandle[timer_num].handle.Init.ClockDivision  = TIM_CLOCKDIVISION_DIV1;
        TimerHandle[timer_num].callback = (uint32_t)TC6_Handler;
        HAL_NVIC_SetPriority(STEP_TIMER_IRQ_ID, ISR_PRIORITY_MOVE_TIMER, 0);
        break;

      case TEMP_TIMER_NUM:
        // TEMP TIMER TIM7 - any available 16bit Timer (1 already used for PWM)
        __HAL_RCC_TIM7_CLK_ENABLE();
        TimerHandle[timer_num].handle.Instance            = TIM7;
        TimerHandle[timer_num].handle.Init.Prescaler      = temp_prescaler;
        TimerHandle[timer_num].handle.Init.CounterMode    = TIM_COUNTERMODE_UP;
        TimerHandle[timer_num].handle.Init.ClockDivision  = TIM_CLOCKDIVISION_DIV1;
        TimerHandle[timer_num].callback = (uint32_t)TC7_Handler;
        HAL_NVIC_SetPriority(TEMP_TIMER_IRQ_ID, ISR_PRIORITY_TEMP_TIMER, 0);
        break;
    }
    timers_initialized[timer_num] = true;
  }

  switch (timer_num) {
    case STEP_TIMER_NUM:
      if (HAL_TIM_OC_Init(&TimerHandle[timer_num].handle) == HAL_OK) {
        TIM_OC_InitTypeDef sConfig = {0};
        sConfig.OCMode     = TIM_OCMODE_TIMING;
        sConfig.Pulse      = 65535; // By default, the stepper routine will be called every 0.065536s until the queue is filled.

        if (HAL_TIM_OC_ConfigChannel(&TimerHandle[timer_num].handle, &sConfig, TIM_CHANNEL_1) == HAL_OK) {
          if (HAL_TIM_OC_Start_IT(&TimerHandle[timer_num].handle, TIM_CHANNEL_1) != HAL_OK)
            fatal_error("HAL_TIM_OC_Start_IT - Failed", "HAL_timer_start");
        } else {
          fatal_error("HAL_TIM_OC_ConfigChannel - Failed", "HAL_timer_start");
        }
      } else {
        fatal_error("HAL_TIM_OC_Init - Failed", "HAL_timer_start");
      }
      break;

  case MOVE_TIMER_NUM:
  case TEMP_TIMER_NUM:
      TimerHandle[timer_num].handle.Init.Period = (((HAL_TIMER_RATE) / TimerHandle[timer_num].handle.Init.Prescaler) / frequency) - 1;
      if (HAL_TIM_Base_Init(&TimerHandle[timer_num].handle) == HAL_OK)
        HAL_TIM_Base_Start_IT(&TimerHandle[timer_num].handle);
      break;
  }
}

extern "C" void TIM1_CC_IRQHandler() {
  ((void(*)())TimerHandle[STEP_TIMER_NUM].callback)();
}
extern "C" void TIM7_IRQHandler() {
  ((void(*)())TimerHandle[TEMP_TIMER_NUM].callback)();
}
extern "C" void TIM6_IRQHandler() {
  ((void(*)())TimerHandle[MOVE_TIMER_NUM].callback)();
}

void HAL_timer_enable_interrupt(const uint8_t timer_num) {
  switch (timer_num) {
    case STEP_TIMER_NUM: HAL_NVIC_EnableIRQ(STEP_TIMER_IRQ_ID); break;
    case MOVE_TIMER_NUM: HAL_NVIC_EnableIRQ(MOVE_TIMER_IRQ_ID); break;
    case TEMP_TIMER_NUM: HAL_NVIC_EnableIRQ(TEMP_TIMER_IRQ_ID); break;
  }
}

void HAL_timer_disable_interrupt(const uint8_t timer_num) {
  switch (timer_num) {
    case STEP_TIMER_NUM: HAL_NVIC_DisableIRQ(STEP_TIMER_IRQ_ID); break;
    case MOVE_TIMER_NUM: HAL_NVIC_DisableIRQ(MOVE_TIMER_IRQ_ID); break;
    case TEMP_TIMER_NUM: HAL_NVIC_DisableIRQ(TEMP_TIMER_IRQ_ID); break;
  }
  // We NEED memory barriers to ensure Interrupts are actually disabled!
  // ( https://dzone.com/articles/nvic-disabling-interrupts-on-arm-cortex-m-and-the )
  __DSB();
  __ISB();
}

bool HAL_timer_interrupt_enabled(const uint8_t timer_num) {
  switch (timer_num) {
    case STEP_TIMER_NUM: return NVIC_GetEnableIRQ(STEP_TIMER_IRQ_ID);
    case MOVE_TIMER_NUM: return NVIC_GetEnableIRQ(MOVE_TIMER_IRQ_ID);
    case TEMP_TIMER_NUM: return NVIC_GetEnableIRQ(TEMP_TIMER_IRQ_ID);
  }
  return false;
}

#endif // STM32GENERIC && STM32F4
