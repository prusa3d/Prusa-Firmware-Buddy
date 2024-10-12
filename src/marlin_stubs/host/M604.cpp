#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "marlin_server.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M604: Abort serial print <a href="https://reprap.org/wiki/G-code#M604:_Abort_serial_print">M604: Abort serial print</a>
 *
 * This is expected to be set as end-print command in host systems like Octoprint
 *
 *#### Usage
 *
 *    M604
 */
void GcodeSuite::M604() {
    marlin_server::print_abort();
}

/** @}*/
