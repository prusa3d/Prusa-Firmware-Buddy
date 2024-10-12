/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"

#if HAS_HEATED_BED

#include "../gcode.h"
#include "../../module/temperature.h"
#include "../../module/motion.h"
#include "../../lcd/ultralcd.h"

#if ENABLED(PRINTJOB_TIMER_AUTOSTART)
  #include "../../module/printcounter.h"
#endif

#if ENABLED(PRINTER_EVENT_LEDS)
  #include "../../feature/leds/leds.h"
#endif

#include "../../Marlin.h" // for wait_for_heatup and idle()

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M140: Set bed temperature <a href="https://reprap.org/wiki/G-code#M140:_Set_Bed_Temperature_.28Fast.29">M140: Set Bed Temperature (Fast)</a>
 *
 *#### Usage
 *
 *    M140 [ S ]
 *
 * #### Parameters
 *
 * - `S` - Set target bed temperature
 */
void GcodeSuite::M140() {
  if (DEBUGGING(DRYRUN)) return;
  if (parser.seenval('S')) thermalManager.setTargetBed(parser.value_celsius());
}

/**
 *### M190: Wait for bed current temp to reach target temp <a href="https://reprap.org/wiki/G-code#M190:_Wait_for_bed_temperature_to_reach_target_temp">M190: Wait for bed temperature to reach target temp</a>
 *
 *#### Usage
 *
 *    M190 [ S | R ]
 *
 *#### Parameters
 *
 * - `S` - Set target bed temperature and waits only when heating
 * - `R` - Set target bed temperature and waits when heating and/or cooling
 */
void GcodeSuite::M190() {
  if (DEBUGGING(DRYRUN)) return;

  const bool no_wait_for_cooling = parser.seenval('S');
  if (no_wait_for_cooling || parser.seenval('R')) {
    thermalManager.setTargetBed(parser.value_celsius());
    #if ENABLED(PRINTJOB_TIMER_AUTOSTART)
      if (parser.value_celsius() > BED_MINTEMP)
        print_job_timer.start();
    #endif
  }
  else return;

  ui.set_status_P(thermalManager.isHeatingBed() ? GET_TEXT(MSG_BED_HEATING) : GET_TEXT(MSG_BED_COOLING));

  thermalManager.wait_for_bed(no_wait_for_cooling);
}

/** @}*/

#endif // HAS_HEATED_BED
