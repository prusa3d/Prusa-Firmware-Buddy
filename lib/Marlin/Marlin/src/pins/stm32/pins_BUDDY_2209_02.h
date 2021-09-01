/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
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

#include "main.h"
#include "hwio_pindef.h"

#if !defined(STM32F4) && !defined(STM32F4xx)
  #error "Oops! Select a Buddy board in 'Tools > Board.'"
#endif

#define DEFAULT_MACHINE_NAME "Prusa-mini"
#define BOARD_NAME "Buddy Board"

#define I2C_EEPROM

#define E2END 0x03ff // EEPROM end address (1kB)


#if HOTENDS > 1 || E_STEPPERS > 2
  #error "Buddy supports up to 1 hotends / E-steppers."
#endif

//
// Limit Switches
//
#define X_MIN_PIN              MARLIN_PIN(X_DIAG)
#define X_MAX_PIN              MARLIN_PIN(X_DIAG)
#define Y_MIN_PIN              MARLIN_PIN(Y_DIAG)
#define Y_MAX_PIN              MARLIN_PIN(Y_DIAG)
#define Z_MIN_PIN              MARLIN_PIN(Z_MIN)
#define Z_MAX_PIN              MARLIN_PIN(Z_DIAG)

//
// Z Probe (when not Z_MIN_PIN)
//

//
// Steppers
//

#define X_STEP_PIN             MARLIN_PIN(X_STEP)
#define X_DIR_PIN              MARLIN_PIN(X_DIR)
#define X_ENABLE_PIN           MARLIN_PIN(X_ENA)

#define Y_STEP_PIN             MARLIN_PIN(Y_STEP)
#define Y_DIR_PIN              MARLIN_PIN(Y_DIR)
#define Y_ENABLE_PIN           MARLIN_PIN(Y_ENA)

#define Z_STEP_PIN             MARLIN_PIN(Z_STEP)
#define Z_DIR_PIN              MARLIN_PIN(Z_DIR)
#define Z_ENABLE_PIN           MARLIN_PIN(Z_ENA)

#define E0_STEP_PIN            MARLIN_PIN(E0_STEP)
#define E0_DIR_PIN             MARLIN_PIN(E0_DIR)
#define E0_ENABLE_PIN          MARLIN_PIN(E0_ENA)


#if HAS_DRIVER(TMC2208)
  /**
   * TMC2208 stepper drivers
   *
   * Hardware serial communication ports.
   * If undefined software serial is used according to the pins below
   */
  #define TMC2208_SERIAL     Serial3
  #define X_HARDWARE_SERIAL  TMC2208_SERIAL
  #define Y_HARDWARE_SERIAL  TMC2208_SERIAL
  #define Z_HARDWARE_SERIAL  TMC2208_SERIAL
  #define E0_HARDWARE_SERIAL TMC2208_SERIAL
#elif HAS_DRIVER(TMC2209)
  #define TMC2209_SERIAL Serial3

  #define X_HARDWARE_SERIAL TMC2209_SERIAL
  #define Y_HARDWARE_SERIAL TMC2209_SERIAL
  #define Z_HARDWARE_SERIAL TMC2209_SERIAL
  #define E0_HARDWARE_SERIAL TMC2209_SERIAL

  #define X_SLAVE_ADDRESS 1
  #define Y_SLAVE_ADDRESS 3
  #define Z_SLAVE_ADDRESS 0
  #define E0_SLAVE_ADDRESS 2
#else
  #error Unknown stepper driver
#endif

//
// Temperature Sensors
//

#define TEMP_0_PIN             MARLIN_PIN(TEMP_0)     // Analog Input
#define TEMP_BED_PIN           MARLIN_PIN(TEMP_BED)   // Analog Input

#define TEMP_BOARD_PIN         MARLIN_PIN(THERM2) // Analog Input


//
// Heaters / Fans
//

#define HEATER_0_PIN           MARLIN_PIN(HEAT0)
#define HEATER_BED_PIN         MARLIN_PIN(BED_HEAT)

#define FAN_PIN                MARLIN_PIN(FAN)

#undef E0_AUTO_FAN_PIN         //todo fixme, remove other definition of E0_AUTO_FAN_PIN
#define E0_AUTO_FAN_PIN        MARLIN_PIN(FAN1)



#define SDSS                   80  //it means "NC"
#define SD_DETECT_PIN          80  //it means "NC"

