/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "common/extended_printer_type.hpp"
#include "Marlin/src/gcode/parser.h"
#include "Marlin/src/gcode/gcode.h"

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

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

    #if ENABLED(GCODE_COMPATIBILITY_MK3) || ENABLED(FAN_COMPATIBILITY_MK4_MK3)
    if (parser.boolval('P')) {
        // detect MK3<anything>
        char *arg = parser.string_arg;
        while (*arg == ' ' || *arg == '\"') {
            arg++;
        }
        #if ENABLED(GCODE_COMPATIBILITY_MK3)
        if (strncmp(arg, "MK3", 3) == 0 && strncmp(arg, "MK3.", 4) != 0) {
            GcodeSuite::gcode_compatibility_mode = GcodeSuite::GcodeCompatibilityMode::MK3;
            #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
            if (config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s) {
                GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S;
            }
            #endif
        }
        #endif

        #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
        if (config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s && ((strncmp(arg, "MK4", 3) == 0 && strncmp(arg, "MK4S", 4) != 0) || // Detect classic MK4
                (strncmp(arg, "MK3", 3) == 0)) // Detect MK3.5 and MK3.9
        ) {
            GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S;
        }
        #endif
    }
    #endif
}

/** @}*/

#endif
