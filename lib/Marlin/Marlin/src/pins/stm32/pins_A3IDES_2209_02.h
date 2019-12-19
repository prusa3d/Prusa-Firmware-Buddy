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
#define X_MIN_PIN              PE2
#define X_MAX_PIN              PE2
#define Y_MIN_PIN              PE1
#define Y_MAX_PIN              PE1
#define Z_MIN_PIN              PA8
#define Z_MAX_PIN              PE3

//
// Z Probe (when not Z_MIN_PIN)
//

//#ifndef Z_MIN_PROBE_PIN
//  #define Z_MIN_PROBE_PIN    PA4
//#endif

//
// Steppers
//

#define X_STEP_PIN             PD1
#define X_DIR_PIN              PD0
#define X_ENABLE_PIN           PD3

#define Y_STEP_PIN             PD13
#define Y_DIR_PIN              PD12
#define Y_ENABLE_PIN           PD14

#define Z_STEP_PIN             PD4
#define Z_DIR_PIN              PD15
#define Z_ENABLE_PIN           PD2

#define E0_STEP_PIN            PD9
#define E0_DIR_PIN             PD8
#define E0_ENABLE_PIN          PD10


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
#endif

//#define SCK_PIN              PA5
//#define MISO_PIN             PA6
//#define MOSI_PIN             PA7

//
// Temperature Sensors
//

#define TEMP_0_PIN             PC0   // Analog Input
#define TEMP_BED_PIN           PA4   // Analog Input

#define TEMP_PINDA_PIN         PA6   // Analog Input


//
// Heaters / Fans
//

#define HEATER_0_PIN           PB1
#define HEATER_BED_PIN         PB0

#define FAN_PIN                PE11

//#define FAN1_PIN               PE9
#undef E0_AUTO_FAN_PIN
#define E0_AUTO_FAN_PIN        PE9

//#define ORIG_E0_AUTO_FAN_PIN   PE9 // Use this by NOT overriding E0_AUTO_FAN_PIN


#define SDSS                   80  //it means "NC"
#define SD_DETECT_PIN          80  //it means "NC"

//#define LED_PIN                PC13        //Alive

//#define PWR_LOSS               PA4         //Power loss / nAC_FAULT

#define BEEPER_PIN             PA0

#define BTN_ENC                PE12
#define BTN_EN1                PE15
#define BTN_EN2                PE13

//#define FIL_RUNOUT_PIN         PA3
