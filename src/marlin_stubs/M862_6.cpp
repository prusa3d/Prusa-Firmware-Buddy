/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "gcode_info.hpp"

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.6: Check supported features <a href="https://reprap.org/wiki/G-code#M862.6:_Firmware_features">M862.6: Firmware features</a>
 *
 *#### Usage
 *
 *    M [ Q | P "<string>" ]
 *
 *#### Parameters
 *
 * - `Q` - Print current supported features
 * - `P "<string>"` - Check for feature
 */
void PrusaGcodeSuite::M862_6() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        char temp_buf[sizeof("  M862.6 P\"01234567890123456789\"")];
        for (auto &feature : GCodeInfo::supported_features) {
            SERIAL_ECHO_START();
            snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.6 P\"%s\""), feature);
            SERIAL_ECHO(temp_buf);
            SERIAL_EOL();
        }
    }
}

/** @}*/

#endif
