/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "../../gcode/gcode.h"

#ifdef PRINT_CHECKING_Q_CMDS
/**
 * M862.3: Check model name
 */
void PrusaGcodeSuite::M862_3() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        SERIAL_ECHO_START();
        SERIAL_ECHO("  M862.3 P \"" PRINTER_MODEL "\"");
        SERIAL_EOL();
    }

    #if ENABLED(GCODE_COMPATIBILITY_MK3)
    if (parser.boolval('P')) {
        // detect MK3<anything>
        char *arg = parser.string_arg;
        while (*arg == ' ' || *arg == '\"') {
            arg++;
        }
        if (strncmp(arg, "MK3", 3) == 0 && strncmp(arg, "MK3.", 4) != 0) {
            gcode.compatibility_mode = GcodeSuite::CompatibilityMode::MK3;
        }
    }
    #endif
}
#endif
