#include <gcode/parser.h>
#include <fsm_network_setup.hpp>

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 * Run wi-fi setup wizard
 *
 * ## Parameters
 *
 * - `A` - (Internal) run the wizard as if part of the initial setup
 * - `I` - (Internal) run only loading INI file
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
