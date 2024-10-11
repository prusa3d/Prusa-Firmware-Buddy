#include <module/planner.h>

#include "PrusaGcodeSuite.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9201: Reset motion parameters to defaults
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    9201
 *
 */
void PrusaGcodeSuite::M9201() {
    Motion_Parameters::reset();
}
/** @}*/
