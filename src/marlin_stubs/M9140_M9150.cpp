#include "PrusaGcodeSuite.hpp"

#include <marlin_vars.hpp>
#include <module/planner.h>

/** \addtogroup G-Codes
 * @{
 */

/**
 * Disable stealth mode (switch to standard mode)
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
 * Enable stealth mode
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
