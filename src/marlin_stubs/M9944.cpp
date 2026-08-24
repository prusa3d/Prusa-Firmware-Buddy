#include <module/planner.h>
#include <common/gcode/gcode_parser.hpp>

#include "PrusaGcodeSuite.hpp"

namespace marlin_server {
void execute_gcode_interrupt();
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9944: Execute g-code interrupt
 *
 * Internal GCode
 * Pure necromancy - see the implementation
 *
 */
void PrusaGcodeSuite::M9944() {
    marlin_server::execute_gcode_interrupt();
}
/** @}*/
