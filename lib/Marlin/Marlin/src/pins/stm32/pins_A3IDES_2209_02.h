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

#if !defined(STM32F4) && !defined(STM32F4xx)
  #error "Oops! Select an A3ides board in 'Tools > Board.'"
#endif

#define DEFAULT_MACHINE_NAME "Prusa-mini"
#define BOARD_NAME "A3ides Board"

#define I2C_EEPROM

#define E2END 0x03ff // EEPROM end address (1kB)


#if HOTENDS > 1 || E_STEPPERS > 2
  #error "A3ides supports up to 1 hotends / E-steppers."
#endif

//
// Limit Switches
//
#define X_MIN_PIN              (X_DIAG_GPIO_Port + X_DIAG_Pin)
#define X_MAX_PIN              (X_DIAG_GPIO_Port + X_DIAG_Pin)
#define Y_MIN_PIN              (Z_DIAGE1_GPIO_Port + Z_DIAGE1_Pin) //todo this name doesn't look right
#define Y_MAX_PIN              (Z_DIAGE1_GPIO_Port + Z_DIAGE1_Pin) //todo this name doesn't look right
#define Z_MIN_PIN              (Z_MIN_GPIO_Port + Z_MIN_Pin)
#define Z_MAX_PIN              (Z_DIAG_GPIO_Port + Z_DIAG_Pin)

//
// Z Probe (when not Z_MIN_PIN)
//

//#ifndef Z_MIN_PROBE_PIN
//  #define Z_MIN_PROBE_PIN    PA4
//#endif

//
// Steppers
//

#define X_STEP_PIN             (X_STEP_GPIO_Port + X_STEP_Pin)
#define X_DIR_PIN              (X_DIR_GPIO_Port + X_DIR_Pin)
#define X_ENABLE_PIN           (X_ENA_GPIO_Port + X_ENA_Pin)

#define Y_STEP_PIN             (Y_STEP_GPIO_Port + Y_STEP_Pin)
#define Y_DIR_PIN              (Y_DIR_GPIO_Port + Y_DIR_Pin)
#define Y_ENABLE_PIN           (Y_ENA_GPIO_Port + Y_ENA_Pin)

#define Z_STEP_PIN             (Z_STEP_GPIO_Port + Z_STEP_Pin)
#define Z_DIR_PIN              (Z_DIR_GPIO_Port + Z_DIR_Pin)
#define Z_ENABLE_PIN           (Z_ENA_GPIO_Port + Z_ENA_Pin)

#define E0_STEP_PIN            (E_STEP_GPIO_Port + E_STEP_Pin)
#define E0_DIR_PIN             (E_DIR_GPIO_Port + E_DIR_Pin)
#define E0_ENABLE_PIN          (E_ENA_GPIO_Port + E_ENA_Pin)


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

#define TEMP_0_PIN             (THERM_0_GPIO_Port + THERM_0_Pin)   // Analog Input
#define TEMP_BED_PIN           (THERM_1_GPIO_Port + THERM_1_Pin)   // Analog Input

#define TEMP_PINDA_PIN         PA6   // Analog Input //todo remove
#define TEMP_BOARD_PIN         (THERM_BOARD_GPIO_Port + THERM_BOARD_Pin) // Analog Input


//
// Heaters / Fans
//

#define HEATER_0_PIN           (HEAT0_GPIO_Port + HEAT0_Pin)
#define HEATER_BED_PIN         (BED_HEAT_GPIO_Port + BED_HEAT_Pin)

#define FAN_PIN                (FAN0_GPIO_Port + FAN0_Pin)

#undef E0_AUTO_FAN_PIN         //todo fixme, remove other definition of E0_AUTO_FAN_PIN
#define E0_AUTO_FAN_PIN        (FAN1_GPIO_Port + FAN1_Pin)

//#define ORIG_E0_AUTO_FAN_PIN   PE9 // Use this by NOT overriding E0_AUTO_FAN_PIN


#define SDSS                   80  //it means "NC"
#define SD_DETECT_PIN          80  //it means "NC"

//#define LED_PIN                PC13        //Alive

//#define PWR_LOSS               PA4         //Power loss / nAC_FAULT

//#define BEEPER_PIN             PA0         //comment to disable macro HAS_BUZZER 

#define BTN_ENC                PE12 //todo remove
#define BTN_EN1                PE15 //todo remove
#define BTN_EN2                PE13 //todo remove

#define FIL_RUNOUT_PIN         PB4 //todo remove
