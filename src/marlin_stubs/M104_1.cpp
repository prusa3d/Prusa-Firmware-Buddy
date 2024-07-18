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
 * M104.1: Early Set Hotend Temperature (preheat, and with stealth mode support)
 *
 * This GCode is used to tell the XL printer the time estimate when a tool will be used next,
 * so that the printer can start preheating the tool in advance.
 *
 * ## Parameters
 * - `P` - <number> - time in seconds till the temperature S is required (in standard mode)
 * - `Q` - <number> - time in seconds till the temperature S is required (in stealth mode)
 * The rest is same as M104
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
