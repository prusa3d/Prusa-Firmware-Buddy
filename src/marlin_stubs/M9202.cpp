#include <module/planner.h>

#include "PrusaGcodeSuite.hpp"
#include <module/prusa/homing_corexy.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9202: Clear precise homing calibrations
 *
 */
void PrusaGcodeSuite::M9202() {
    corexy_clear_homing_calibration();
}
/** @}*/
