/**
 * @file
 */
#include "src/module/prusa/toolchanger.h"
#include "../gcode.h"
#include "PrusaGcodeSuite.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### P0: Park extruder (tool)
 *
 * Internal GCode
 *
 * Only XL
 *
 *#### Usage
 *
 *    P0
 *
 *#### Parameters
 *
 * - `F` - [units/min] Set the movement feedrate
 * - `S1` - Don't move the tool in XY after change
 */
void PrusaGcodeSuite::P0() {
    GcodeSuite::T(PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
}
/** @}*/
