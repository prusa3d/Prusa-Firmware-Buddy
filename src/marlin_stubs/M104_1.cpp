/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include <marlin_vars.hpp>
#include <Marlin/src/gcode/parser.h>
#include <gcode/gcode.h>

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M104.1: Early Set Hotend Temperature <a href="https://reprap.org/wiki/G-code#M104.1:_Early_Set_Hotend_Temperature">M104.1: Early Set Hotend Temperature</a>
 *
 * This GCode is used to tell the XL printer the time estimate when a tool will be used next,
 * so that the printer can start preheating the tool in advance.
 *
 *#### Usage
 *
 *   M104.1 [ S | R | D | F | T | P | Q ]
 *
 *#### Parameters
 *
 * - `S` - Wait for extruder(s) to reach temperature. Waits only when heating.
 * - `R` - Wait for extruder(s) to reach temperature. Waits when heating and cooling.
 * - `D` - Display temperature (otherwise actual temp will be displayed)
 * - `F` - Autotemp flag.
 * - `T` - Tool
 * - `P` - Time in seconds till the temperature S is required (in standard mode)
 * - `Q` - Time in seconds till the temperature S is required (in stealth mode)
 *
 */
void PrusaGcodeSuite::M104_1() {
    const bool is_stealth_mode = marlin_vars().stealth_mode.get();

    // We will be applying a thermal model in the future to better evaluate when the tool should start being heated.
    // For now, we start heating as soon as we see the gcode and ignore the time data.
    const bool should_start_preheating = //
        (parser.seen('P') && !is_stealth_mode)
        || (parser.seen('Q') && is_stealth_mode);

    // If we should start preheating, simply process this command as if it was standard M104 (otherwise do nothing)
    if (should_start_preheating) {
        GcodeSuite::M104();
    }
}

/** @}*/

#endif
