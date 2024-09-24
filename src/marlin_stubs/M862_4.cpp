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
 * M862.4: Check firmware version
 *
 * ## Parameters
 *
 * - `Q` - Print out current firmware version
 */
void PrusaGcodeSuite::M862_4() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        SERIAL_ECHO_START();
        char temp_buf[sizeof("  M862.4 P0123456789")];
        char version_buffer[8] {};
        version::fill_project_version_no_dots(version_buffer, sizeof(version_buffer));
        snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.4 P%s"), version_buffer);
        SERIAL_ECHO(temp_buf);
        SERIAL_EOL();
    }
}

/** @}*/

#endif
