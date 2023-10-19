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

#include "hwio_pindef.h"
#include <device/board.h>

#if !defined(STM32F4) && !defined(STM32F4xx)
  #error "Oops! Select a Buddy board in 'Tools > Board.'"
#endif

#define DEFAULT_MACHINE_NAME "Prusa-XL"
#define BOARD_NAME "Buddy Board"

#define I2C_EEPROM

#define E2END 0x03ff // EEPROM end address (1kB)

#if HOTENDS > 6 || E_STEPPERS > 2
  #error "XLBuddy supports up to 5+1 hotends / E-steppers."
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

#define Z_STEP_PIN             MARLIN_PIN(Z_STEP)
#define Z_DIR_PIN              MARLIN_PIN(Z_DIR)
#define Z_ENABLE_PIN           MARLIN_PIN(Z_ENA)

#define E0_STEP_PIN            MARLIN_PIN(E0_STEP)
#define E0_DIR_PIN             MARLIN_PIN(E0_DIR)
#define E0_ENABLE_PIN          MARLIN_PIN(E0_ENA)


#define E1_ENABLE_PIN          MARLIN_PIN(E1_ENA)
#define E2_ENABLE_PIN          MARLIN_PIN(E2_ENA)
#define E3_ENABLE_PIN          MARLIN_PIN(E3_ENA)
#define E4_ENABLE_PIN          MARLIN_PIN(E4_ENA)

#if !HAS_DRIVER(TMC2130)
#error TMC2130 is expected
#endif

#define X_CS_PIN MARLIN_PIN(CS_X)
#define Y_CS_PIN MARLIN_PIN(CS_Y)
#define Z_CS_PIN MARLIN_PIN(CS_Z)
#define E0_CS_PIN MARLIN_PIN(CS_E)



//
// Temperature Sensors
//
#define TEMP_0_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy
#define TEMP_1_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy
#define TEMP_2_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy
#define TEMP_3_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy
#define TEMP_4_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy
#define TEMP_5_PIN             MARLIN_PIN(TEMP_0)     // Dummy, measured by puppy



#define TEMP_BED_PIN           MARLIN_PIN(TEMP_BED)   // Dummy, measured by puppy

#define TEMP_BOARD_PIN         MARLIN_PIN(THERM2) // Dummy, measured by puppy
#define TEMP_HEATBREAK_PIN     MARLIN_PIN(TEMP_HEATBREAK) // Dummy, measured by puppy
#define TEMP_CHAMBER_PIN       MARLIN_PIN(AMBIENT) // Analog Input

#define HEATER_HEATBREAK_PIN   MARLIN_PIN(FAN1)
#define FAN1_PIN               MARLIN_PIN(FAN1)

//
// Heaters / Fans
//

#define HEATER_0_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy
#define HEATER_1_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy
#define HEATER_2_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy
#define HEATER_3_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy
#define HEATER_4_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy
#define HEATER_5_PIN           MARLIN_PIN(HEAT0) // Dummy, measured by puppy

#define HEATER_BED_PIN         MARLIN_PIN(BED_HEAT) // Dummy, measured by puppy

#define FAN_PIN                MARLIN_PIN(FAN)


#undef E0_AUTO_FAN_PIN         // no auto fan pin - handled by dwarf


#define SDSS                   80  //it means "NC"
#define SD_DETECT_PIN          80  //it means "NC"
