#include <module/planner.h>
#include <common/gcode/gcode_parser.hpp>
#include <feature/cork/tracker.hpp>

#include "PrusaGcodeSuite.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9933: Internal Cork
 *
 * Internal GCode
 *
 * Marks the given cork as done, signalling that all g-codes submitted before it
 * have finished executing. Waits for the motion planner to drain first.
 * Cookies not handed out by the cork tracker are ignored.
 *
 *#### Usage
 *
 *    M9933 [ C ]
 *
 *#### Parameters
 *
 * - `C` - [value] Cookie identifying the cork
 *
 *#### Examples
 *
 *    M9933 C41337 ; Mark the cork with cookie 41337 as done
 *
 */
void PrusaGcodeSuite::M9933() {
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

    if (auto cookie = parser.option<buddy::cork::Tracker::Cookie>('C'); cookie.has_value()) {
        planner.synchronize();
        buddy::cork::tracker.mark_done(*cookie);
    }
}
/** @}*/
