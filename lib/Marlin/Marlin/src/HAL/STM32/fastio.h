/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 * Copyright (c) 2017 Victor Perez
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
#pragma once

/**
 * Fast I/O interfaces for STM32F4/7
 * These use GPIO functions instead of Direct Port Manipulation, as on AVR.
 */

// ------------------------
// Public Variables
// ------------------------

extern GPIO_TypeDef * FastIOPortMap[];

// ------------------------
// Public functions
// ------------------------

void FastIO_init(); // Must be called before using fast io macros
#define FASTIO_INIT() FastIO_init()

// ------------------------
// Defines
// ------------------------

#ifndef PWM
  #define PWM OUTPUT
#endif

#define READ(IO)                digitalRead(IO)
#define WRITE(IO,V)             digitalWrite(IO,V)
 
#define _GET_MODE(IO)
#define _SET_MODE(IO,M)         pinMode(IO, M)
#define _SET_OUTPUT(IO)         pinMode(IO, OUTPUT)                               /*!< Output Push Pull Mode & GPIO_NOPULL   */
 
#define OUT_WRITE(IO,V)         do{ _SET_OUTPUT(IO); WRITE(IO,V); }while(0)
 
#define SET_INPUT(IO)           _SET_MODE(IO, INPUT)                              /*!< Input Floating Mode                   */
#define SET_INPUT_PULLUP(IO)    _SET_MODE(IO, INPUT_PULLUP)                       /*!< Input with Pull-up activation         */
#define SET_INPUT_PULLDOWN(IO)  _SET_MODE(IO, INPUT_PULLDOWN)                     /*!< Input with Pull-down activation       */
#define SET_OUTPUT(IO)          OUT_WRITE(IO, LOW)
#define SET_PWM(IO)             _SET_MODE(IO, PWM)
 
#define TOGGLE(IO)              OUT_WRITE(IO, !READ(IO))
 
#define IS_INPUT(IO)
#define IS_OUTPUT(IO)
 
#define PWM_PIN(P)              true

// digitalRead/Write wrappers
#define extDigitalRead(IO)    digitalRead(IO)
#define extDigitalWrite(IO,V) digitalWrite(IO,V)

//
// Pins Definitions
//

#undef GPIOA
#define GPIOA (0 * 16)
#undef GPIOB
#define GPIOB (1 * 16)
#undef GPIOC
#define GPIOC (2 * 16)
#undef GPIOD
#define GPIOD (3 * 16)
#undef GPIOE
#define GPIOE (4 * 16)
#undef GPIOF
#define GPIOF (5 * 16)
#undef GPIOG
#define GPIOG (6 * 16)

#undef GPIO_PIN_0
#define GPIO_PIN_0 0
#undef GPIO_PIN_1
#define GPIO_PIN_1 1
#undef GPIO_PIN_2
#define GPIO_PIN_2 2
#undef GPIO_PIN_3
#define GPIO_PIN_3 3
#undef GPIO_PIN_4
#define GPIO_PIN_4 4
#undef GPIO_PIN_5
#define GPIO_PIN_5 5
#undef GPIO_PIN_6
#define GPIO_PIN_6 6
#undef GPIO_PIN_7
#define GPIO_PIN_7 7
#undef GPIO_PIN_8
#define GPIO_PIN_8 8
#undef GPIO_PIN_9
#define GPIO_PIN_9 9
#undef GPIO_PIN_10
#define GPIO_PIN_10 10
#undef GPIO_PIN_11
#define GPIO_PIN_11 11
#undef GPIO_PIN_12
#define GPIO_PIN_12 12
#undef GPIO_PIN_13
#define GPIO_PIN_13 13
#undef GPIO_PIN_14
#define GPIO_PIN_14 14
#undef GPIO_PIN_15
#define GPIO_PIN_15 15
