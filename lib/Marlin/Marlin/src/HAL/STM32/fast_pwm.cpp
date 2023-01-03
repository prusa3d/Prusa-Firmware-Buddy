/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../platforms.h"

#ifdef HAL_STM32

#include "../../inc/MarlinConfig.h"

// Array to support sticky frequency sets per timer
static uint16_t timer_freq[TIMER_NUM];

void MarlinHAL::set_pwm_frequency(const pin_t pin, const uint16_t f_desired) {
  if (!PWM_PIN(pin)) return; // Don't proceed if no hardware timer
  const PinName pin_name = digitalPinToPinName(pin);
  TIM_TypeDef * const Instance = (TIM_TypeDef *)pinmap_peripheral(pin_name, PinMap_PWM); // Get HAL timer instance
  const timer_index_t index = get_timer_index(Instance);

  // Protect used timers.
  #ifdef STEP_TIMER
    if (index == TIMER_INDEX(STEP_TIMER)) return;
  #endif
  #ifdef TEMP_TIMER
    if (index == TIMER_INDEX(TEMP_TIMER)) return;
  #endif
  #if defined(PULSE_TIMER) && MF_TIMER_PULSE != MF_TIMER_STEP
    if (index == TIMER_INDEX(PULSE_TIMER)) return;
  #endif

  if (HardwareTimer_Handle[index] == nullptr) // If frequency is set before duty we need to create a handle here.
    HardwareTimer_Handle[index]->__this = new HardwareTimer((TIM_TypeDef *)pinmap_peripheral(pin_name, PinMap_PWM));
  HardwareTimer * const HT = (HardwareTimer *)(HardwareTimer_Handle[index]->__this);
  HT->setOverflow(f_desired, HERTZ_FORMAT);
  timer_freq[index] = f_desired; // Save the last frequency so duty will not set the default for this timer number.
}

#endif // HAL_STM32
