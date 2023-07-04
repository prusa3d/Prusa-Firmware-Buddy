/**
 * @file
 */
#include "src/module/prusa/toolchanger.h"
#include "../gcode.h"
#include "PrusaGcodeSuite.hpp"

/**
 * Park extruder (tool)
 *
 * ## Parameters
 *
 * - `F` - [units/min] Set the movement feedrate
 * - `S1` - Don't move the tool in XY after change
 */
void PrusaGcodeSuite::P0() {
    GcodeSuite::T(PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
}
