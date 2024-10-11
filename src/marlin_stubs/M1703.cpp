#include <gcode/parser.h>
#include <fsm_network_setup.hpp>

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1703: Run wi-fi setup wizard
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1703 [ A | I ]
 *
 *#### Parameters
 *
 * - `A` - Run the wizard as if part of the initial setup
 * - `I` - Run only loading INI file
 */
void M1703() {
    if (parser.seen('A')) {
        network_wizard::network_initial_setup_wizard();

    } else if (parser.seen('I')) {
        network_wizard::network_ini_wizard();

    } else {
        network_wizard::network_setup_wizard();
    }
}

/** @}*/

} // namespace PrusaGcodeSuite
