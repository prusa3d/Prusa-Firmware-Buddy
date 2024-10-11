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

#if DISABLED(EMERGENCY_PARSER)

#include "../gcode.h"
#include "../../Marlin.h" // for wait_for_heatup, kill, quickstop_stepper

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M108: Cancel Heating <a href="https://reprap.org/wiki/G-code#M108:_Cancel_Heating">M108: Cancel Heating</a>
 *
 * Stop the waiting for heaters in M109, M190, M303. Does not affect the target temperature
 *
 *#### Usage
 *
 *    M108
 *
 */
void GcodeSuite::M108() {
  #if HAS_RESUME_CONTINUE
    wait_for_user = false;
  #endif
  wait_for_heatup = false;
}

/**
 *### M112: Full Shutdown <a href="https://reprap.org/wiki/G-code#M112:_Full_.28Emergency.29_Stop">M112: Full (Emergency) Stop</a>
 *
 *#### Usage
 *
 *    M112
 *
 */
void GcodeSuite::M112() {
  kill(PSTR("Emergency stop (M112)"), nullptr, true);
}

/**
 *### M410: Quickstop - Abort all planned moves <a href="https://reprap.org/wiki/G-code#M410:_Quick-Stop">M410: Quick-Stop</a>
 *
 * This will stop the carriages mid-move, so most likely they
 * will be out of sync with the stepper position after this.
 *
 *#### Usage
 *
 *    M410
 *
 */
void GcodeSuite::M410() {
  quickstop_stepper();
}

/** @}*/

#endif // !EMERGENCY_PARSER
