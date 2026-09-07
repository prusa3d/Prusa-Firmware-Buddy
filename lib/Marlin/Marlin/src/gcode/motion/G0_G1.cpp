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

#include "../gcode.h"
#include "../../module/motion.h"
#include <option/has_crash_detection.h>

#if ENABLED(NANODLP_Z_SYNC)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../../module/planner.h"
#endif

#if HAS_CRASH_DETECTION()
  #include <feature/prusa/crash_recovery.hpp>
#endif

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
  #include <feature/cancel_object/cancel_object.hpp>
#endif

extern xyze_pos_t destination;

#if ENABLED(VARIABLE_G0_FEEDRATE)
  // #error dead code found by automatic analyses (see BFW-5461)
  feedRate_t fast_move_feedrate = MMM_TO_MMS(G0_FEEDRATE);
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G0, G1: Coordinated movement of X Y Z E axes <a href="https://reprap.org/wiki/G-code#G0_.26_G1:_Move">G0 & G1: Move</a>
 *
 *#### Usage
 *
 *    G0 [ X | Y | Z | E | F ]
 *    G1 [ X | Y | Z | E | F ]
 *
 *#### Parameters
 *
 *  - `X` - The position to move to on the X axis
 *  - `Y` - The position to move to on the Y axis
 *  - `Z` - The position to move to on the Z axis
 *  - `E` - The amount to extrude between the starting point and ending point
 *  - `F` - The feedrate per minute of the move between the starting point and ending point (if supplied)
 */
void GcodeSuite::G0_G1(TERN_(HAS_FAST_MOVES, const bool fast_move/*=false*/)) {
  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_RUNNING));

  #if HAS_CRASH_DETECTION()
    // allow full instruction recovery
    crash_s.set_gcode_replay_flags(Crash_s::RECOVER_FULL);
  #endif

  #ifdef G0_FEEDRATE
    // #error dead code found by automatic analyses (see BFW-5461)
    feedRate_t old_feedrate;
    #if ENABLED(VARIABLE_G0_FEEDRATE)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (fast_move) {
        old_feedrate = feedrate_mm_s;             // Back up the (old) motion mode feedrate
        feedrate_mm_s = fast_move_feedrate;       // Get G0 feedrate from last usage
      }
    #endif
  #endif

  get_destination_from_command();                 // Get X Y [Z[I[J[K]]]] [E] F (and set cutter power)

  #if HAS_CANCEL_OBJECT()
    // !!! MUST BE after get_destination_from_command
    // get_destination_from_command can also change the feedrate, effect of which we should keep
    if (buddy::cancel_object().is_current_object_cancelled()) {
      return;
    }
  #endif

  #ifdef G0_FEEDRATE
    // #error dead code found by automatic analyses (see BFW-5461)
    if (fast_move) {
      #if ENABLED(VARIABLE_G0_FEEDRATE)
        // #error dead code found by automatic analyses (see BFW-5461)
        fast_move_feedrate = feedrate_mm_s;       // Save feedrate for the next G0
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        old_feedrate = feedrate_mm_s;             // Back up the (new) motion mode feedrate
        feedrate_mm_s = MMM_TO_MMS(G0_FEEDRATE);  // Get the fixed G0 feedrate
      #endif
    }
  #endif

  #if ANY(POLAR)
    #error Dead code
  #else
    prepare_move_to(destination, feedrate_mm_s, { .move = { .is_printing_move = true } });
  #endif

  #ifdef G0_FEEDRATE
    // #error dead code found by automatic analyses (see BFW-5461)
    // Restore the motion mode feedrate
    if (fast_move) feedrate_mm_s = old_feedrate;
  #endif

  #if ENABLED(NANODLP_Z_SYNC)
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(NANODLP_ALL_AXIS)
      // #error dead code found by automatic analyses (see BFW-5461)
      #define _MOVE_SYNC parser.seenval('X') || parser.seenval('Y') || parser.seenval('Z')  // For any move wait and output sync message
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define _MOVE_SYNC parser.seenval('Z')  // Only for Z move
    #endif
    if (_MOVE_SYNC) {
      planner.synchronize();
      SERIAL_ECHOLNPGM(STR_Z_MOVE_COMP);
    }
    TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_IDLE));
  #else
    TERN_(FULL_REPORT_TO_HOST_FEATURE, report_current_grblstate_moving());
  #endif
}

/** @}*/
