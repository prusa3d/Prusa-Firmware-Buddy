/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/parser.h"
#include "gcode/gcode.h"
#include "gcode_info.hpp"
#include "common/printer_model.hpp"

#ifdef PRINT_CHECKING_Q_CMDS

static void setup_gcode_compatibility(const PrinterModelInfo *gcode_printer) {
    // Failed to identify the printer -> do nothing
    if (!gcode_printer) {
        return;
    }

    [[maybe_unused]] const PrinterGCodeCompatibilityReport compatibility = PrinterModelInfo::current().gcode_compatibility_report(*gcode_printer);

    #if ENABLED(GCODE_COMPATIBILITY_MK3)
    if (compatibility.mk3_compatibility_mode) {
        GcodeSuite::gcode_compatibility_mode = GcodeSuite::GcodeCompatibilityMode::MK3;
    }
    #endif

    #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
    if (compatibility.mk4s_fan_compatibility_mode) {
        GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S;
    }
    #endif
}

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
        snprintf(temp_buf, sizeof(temp_buf), PSTR("  M862.2 P%lu"), (unsigned long)PrinterModelInfo::current().gcode_check_code);
        SERIAL_ECHO(temp_buf);
        SERIAL_EOL();
    }

    if (parser.boolval('P')) {
        setup_gcode_compatibility(PrinterModelInfo::from_gcode_check_code(parser.value_int()));
    }
}

/**
 * M862.3: Check model name
 */
void PrusaGcodeSuite::M862_3() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (parser.boolval('Q')) {
        SERIAL_ECHO_START();
        SERIAL_ECHO("  M862.3 P \"");
        SERIAL_ECHO(PrinterModelInfo::current().id_str);
        SERIAL_ECHO("\"");
        SERIAL_EOL();
    }

    if (parser.boolval('P')) {
        const char *arg = parser.string_arg;
        while (*arg == ' ' || *arg == '\"') {
            arg++;
        }

        const char *arg_end = arg;
        while (isalnum(*arg_end) || *arg_end == '.') {
            arg_end++;
        }

        setup_gcode_compatibility(PrinterModelInfo::from_id_str(std::string_view(arg, arg_end)));
    }
}

/** @}*/

#endif
