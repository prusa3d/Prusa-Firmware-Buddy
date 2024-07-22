/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/parser.h"
#include "gcode/gcode.h"
#include "gcode_info.hpp"
#include "common/printer_model.hpp"

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

    #if ENABLED(GCODE_COMPATIBILITY_MK3) || ENABLED(FAN_COMPATIBILITY_MK4_MK3)
    if (parser.boolval('P')) {
        const auto code = printer::model_code_without_mmu(static_cast<printer::PrinterModelCode>(parser.value_int()));

        #if ENABLED(GCODE_COMPATIBILITY_MK3)
        if (printer::requires_gcode_compatibility_mode(code)) {
            GcodeSuite::gcode_compatibility_mode = GcodeSuite::GcodeCompatibilityMode::MK3;
            #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
            if (config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s) {
                GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S;
            }
            #endif
        }
        #endif

        #if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
        if (config_store().extended_printer_type.get() == ExtendedPrinterType::mk4s && printer::requires_fan_compatibility_mode(code)) {
            GcodeSuite::fan_compatibility_mode = GcodeSuite::FanCompatibilityMode::MK3_TO_MK4_NON_S;
        }
        #endif
    }

    #endif
}

/** @}*/

#endif
