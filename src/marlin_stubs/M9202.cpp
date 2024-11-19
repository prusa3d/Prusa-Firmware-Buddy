
#include "PrusaGcodeSuite.hpp"
#include <feature/emergency_stop/emergency_stop.hpp>
#include <common/marlin_server.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9202: Wait for emergency stop conditions to pass.
 *
 * Can be called even outside of emergency stop conditions (in which case it'll
 * just do nothing).
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9201
 *
 */
void PrusaGcodeSuite::M9202() {
    buddy::emergency_stop().gcode_body();
}
