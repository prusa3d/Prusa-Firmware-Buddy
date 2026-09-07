#include <marlin_stubs/PrusaGcodeSuite.hpp>
#include <heaters_selftest.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1987: Heater Selftest
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1987
 *
 */
void PrusaGcodeSuite::M1987() {
    heaters_selftest::run();
}

/** @}*/
