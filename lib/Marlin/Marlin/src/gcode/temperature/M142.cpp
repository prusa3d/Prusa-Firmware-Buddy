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

#if HAS_TEMP_HEATBREAK

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
 *### M142: Set heatbreak cooling temperature <a href="https://reprap.org/wiki/G-code#M142:_Set_Heatbreak_Temperature">M142: Set heatbreak cooling temperature</a>
 *
 *#### Usage
 *
 *    M142 [ S | T ]
 *
 *#### Parameters
 *
 * - `S` - Set heatbreak temperature to be cooled
 * - `T` - Tool
 */
void GcodeSuite::M142() {
  if (DEBUGGING(DRYRUN)) return;

  const int8_t target_extruder = get_target_extruder_from_command();
  if (target_extruder < 0) return;

  if (parser.seenval('S')) thermalManager.setTargetHeatbreak(parser.value_celsius(), target_extruder);
}

/** @}*/

#endif // HAS_TEMP_HEATBREAK
