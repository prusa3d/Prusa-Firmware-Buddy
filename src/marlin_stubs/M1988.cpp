#include <option/has_gantry_squareness_check.h>
#include <marlin_stubs/PrusaGcodeSuite.hpp>
#include <feature/indx_gantry_squareness/indx_gantry_squareness.hpp>
#include <gcode/gcode.h>

static_assert(HAS_GANTRY_SQUARENESS_CHECK());

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1988: Gantry squareness check
 *
 * Measures the gantry skew by stalling the empty head against the outermost
 * INDX docks and runs the result wizard.
 *
 *#### Usage
 *
 *    M1988 [ S ]
 *
 *#### Parameters
 *
 * - `S` - Silent: no GUI and no stored result, just measure and log the result
 */
void PrusaGcodeSuite::M1988() {
    if (parser.seen('S')) {
        indx_gantry_squareness::run_silent();
    } else {
        indx_gantry_squareness::run_wizard();
    }
}

/** @}*/
