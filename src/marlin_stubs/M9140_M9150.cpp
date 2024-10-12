#include "PrusaGcodeSuite.hpp"

#include <marlin_vars.hpp>
#include <module/planner.h>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9140: Disable stealth mode
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9140
 *
 * This affects time estimation and machine motion limits
 */
void PrusaGcodeSuite::M9140() {
    config_store().stealth_mode.set(false);
    marlin_vars().stealth_mode = false;
    planner.set_stealth_mode(false);

    SERIAL_ECHOLNPGM("Stealth mode disabled");
}

/**
 *### M9150: Enable stealth mode
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9150
 *
 *#### Parameters
 *
 *
 * This affects time estimation and machine motion limits
 */
void PrusaGcodeSuite::M9150() {
    config_store().stealth_mode.set(true);
    marlin_vars().stealth_mode = true;
    planner.set_stealth_mode(true);

    SERIAL_ECHOLNPGM("Stealth mode enabled");
}

/** @}*/
