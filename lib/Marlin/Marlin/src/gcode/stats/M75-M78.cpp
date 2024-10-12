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

#include "../gcode.h"
#include "../../module/printcounter.h"
#include "../../lcd/ultralcd.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M75: Start the print job timer <a href="https://reprap.org/wiki/G-code#M75:_Start_the_print_job_timer">M75: Start the print job timer</a>
 *
 *#### Usage
 *
 *    M75
 *
 */
void GcodeSuite::M75() {
  print_job_timer.start();
}

/**
 *### M76: Pause the print job timer <a href="https://reprap.org/wiki/G-code#M76:_Pause_the_print_job_timer">M76: Pause the print job timer</a>
 *
 *#### Usage
 *
 *    M76
 *
 */
void GcodeSuite::M76() {
  print_job_timer.pause();
}

/**
 *### M77: Stop the print job timer <a href="https://reprap.org/wiki/G-code#M77:_Stop_the_print_job_timer">M77: Stop the print job timer</a>
 *
 *#### Usage
 *
 *    M77
 *
 */
void GcodeSuite::M77() {
 print_job_timer.stop();
}

#if ENABLED(PRINTCOUNTER)

/**
 *### M78: Show statistical information about the print jobs <a href="https://reprap.org/wiki/G-code#M78:_Show_statistical_information_about_the_print_jobs">M78: Show statistical information about the print jobs</a>
 *
 * Not active
 *
 *#### Usage
 *
 *    M78 [ S | R ]
 *
 *#### Parameters
 *
 *  - `S78` - Reset the statistics
 *  - `R` - Reset service interval
 *
 *#### Examples
 *
 *    M78     ; Show statistics
 *    M78 S78 ; Reset statistics
 *    M78 R   ; Reset service interval
 */
void GcodeSuite::M78() {
  if (parser.intval('S') == 78) {
    print_job_timer.initStats();
    ui.reset_status();
    return;
  }

  #if HAS_SERVICE_INTERVALS
    if (parser.seenval('R')) {
      print_job_timer.resetServiceInterval(parser.value_int());
      ui.reset_status();
      return;
    }
  #endif

  print_job_timer.showStats();
}

/** @}*/

#endif // PRINTCOUNTER
