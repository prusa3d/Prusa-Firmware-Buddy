/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include <version/version.hpp>

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.4: Check firmware version <a href="https://reprap.org/wiki/G-code#M862.4:_Firmware_version">M862.4: Firmware version</a>
 *
 *#### Usage
 *
 *    M862.4 [ Q | P "<string>" ]
 *
 *#### Parameters
 *
 * - `Q` - Print the current firmware version
 * - `P "<string>"` - Check current firmware version
 */
void PrusaGcodeSuite::M862_4() {
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    char version_buffer[8] {};
    version::fill_project_version_no_dots(version_buffer, sizeof(version_buffer));
    if (parser.boolval('Q')) {
        char temp_buf[sizeof("  M862.4 P0123456789")];
        snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.4 P%s"), version_buffer);
        SERIAL_ECHOLN(temp_buf);
    }
    if (parser.seenval('P')) {
        auto req_version = parser.value_int();
        SERIAL_ECHO("Firmware version ");
        if (atoi(version_buffer) == req_version) {
            SERIAL_ECHO("matches");
        } else {
            SERIAL_ECHO("mismatch");
        }
        SERIAL_ECHOLN(".");
    }
}

/** @}*/

#endif
