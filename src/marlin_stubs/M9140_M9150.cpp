#include "PrusaGcodeSuite.hpp"

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
    planner.synchronize();
    planner.set_stealth_mode(false);

    SERIAL_ECHOLNPGM("Stealth mode disabled");
}

/**
 *### M9141: Stealth mode status
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9141
 *
 * Outputs the current status of stealth mode
 */
void PrusaGcodeSuite::M9141() {
    if (config_store().stealth_mode.get()) {
        SERIAL_ECHOLNPGM("Stealth mode is enabled");
    } else {
        SERIAL_ECHOLNPGM("Stealth mode is disabled");
    }
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
    planner.synchronize();
    planner.set_stealth_mode(true);

    SERIAL_ECHOLNPGM("Stealth mode enabled");
}

/** @}*/
