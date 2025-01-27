/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "gcode_info.hpp"

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 * M862.2: Check model code
 */
void PrusaGcodeSuite::M862_2() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        SERIAL_ECHO_START();
        char temp_buf[sizeof("  M862.2 P0123456789")];
        snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.2 P%lu"), GCodeInfo::getInstance().getPrinterModelCode());
        SERIAL_ECHO(temp_buf);
        SERIAL_EOL();
    }
}

/** @}*/

#endif
