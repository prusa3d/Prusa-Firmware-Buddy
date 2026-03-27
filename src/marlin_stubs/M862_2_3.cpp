/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/parser.h"
#include "gcode/gcode.h"
#include "common/printer_model.hpp"
#include <gcode_info.hpp>
#include <scope_guard.hpp>
#include <module/planner.h>
#include <option/has_gcode_compatibility.h>

#include <option/has_chamber_api.h>
#include <cctype>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
    #include <marlin_stubs/feature/chamber/M141_M191.hpp>
#endif

#ifdef PRINT_CHECKING_Q_CMDS

using namespace buddy;

static PrinterGCodeCompatibilityReport setup_gcode_compatibility(const PrinterModelInfo *gcode_printer) {
    // Failed to identify the printer -> do nothing
    if (!gcode_printer) {
        return {};
    }

    const auto compatibility = PrinterModelInfo::current().gcode_compatibility_report(*gcode_printer);
    gcode.compatibility = compatibility;

    #if HAS_CHAMBER_API() && HAS_GCODE_COMPATIBILITY()
    if (gcode.compatibility.chamber_compatibility_mode) {
        FilamentTypeParameters params;

        // Try to somewhat deduce chamber temperature from the loaded filaments from whatever extruder is used
        GCodeInfo::getInstance().for_each_used_extruder([&]([[maybe_unused]] uint8_t logical_ix, uint8_t physical_ix, const GCodeInfo::ExtruderInfo &) {
            params = config_store().get_filament_type(physical_ix).parameters();
        });

        // If the maximum chamber temperature is specified, enqueue cooling
        if (const auto &t = params.chamber_max_temperature; t.has_value() && chamber().current_temperature() > *t) {
            PrusaGcodeSuite::M141_no_parser({ .target_temp = (buddy::Temperature)*t, .wait_for_cooling = true });
        }

        if (const auto &t = params.chamber_min_temperature; t.has_value() && chamber().current_temperature() < *t) {
        #if PRINTER_IS_PRUSA_COREONE()
            ScopeGuard restore_bed_temp = [temp = thermalManager.degTargetBed()] {
                thermalManager.setTargetBed(temp);
            };

            // C1 does heating using the bed -> set the bed temp to a decently high value
            thermalManager.setTargetBed(100);

            // Home so that we can set the bed to correct postion
            GcodeSuite::G28_no_parser(true, true, true,
                {
                    .only_if_needed = true,
                    .precise = false, // We don't need to be precisely homed for this
                });

            // Move the bed up to reduce chamber volume
            do_blocking_move_to_z(10);
        #else
            #error Please implement heating logic for this printer
        #endif

            PrusaGcodeSuite::M141_no_parser({ .target_temp = (buddy::Temperature)*t, .wait_for_heating = true });
        }

        chamber().set_target_temperature(params.chamber_target_temperature);
    }
    #endif
    return compatibility;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.2: Check model code <a href="https://reprap.org/wiki/G-code#M862.2:_Check_model_code">M862.2: Check model code</a>
 *
 *#### Usage
 *
 *    M862.2 [ Q | P ]
 *
 *#### Parameters
 *
 * - `Q` - Print the current nozzle configuration
 * - `P` - Check printer model number
 *   - `120` - MINI
 *   - `230` - MK3.5
 *     - `30230` - MK3.5MMU3
 *   - `280` - MK3.5S
 *     - `30280` - MK3.5SMMU3
 *   - `210` - MK3.9
 *     - `30210` - MK3.9MMU3
 *   - `270` - MK3.9S
 *     - `30270` - MK3.9SMMU3
 *   - `130` - MK4
 *     - `30130` - MK4MMU3
 *   - `260` - MK4S
 *     - `30260` - MK4SMMU3
 *   - `170` - XL
 *   - `160` - iX
 */

static void print_compatibility(PrinterGCodeCompatibilityReport compatibility) {
    SERIAL_ECHO("Printer is ");
    if (!compatibility.is_compatible) {
        SERIAL_ECHO("in");
    }
    SERIAL_ECHOLN("compatible.");
    if (compatibility.is_compatible) {
        SERIAL_ECHO("Compatibility mode: ");
        if (compatibility.mk3_compatibility_mode) {
            SERIAL_ECHOLN("MK3");
        }
        if (compatibility.mk4_compatibility_mode) {
            SERIAL_ECHOLN("MK4");
        }
        if (compatibility.chamber_compatibility_mode) {
            SERIAL_ECHOLN("CHAMBER");
        }
    }
}

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

    if (parser.seenval('P')) {
        print_compatibility(setup_gcode_compatibility(PrinterModelInfo::from_gcode_check_code(parser.value_int())));
    }
}

/**
 *### M862.3: Check model name <a href="https://reprap.org/wiki/G-code#M862.3:_Model_name">M862.3: Model name</a>
 *
 *#### Usage
 *
 *    M862.3 [ Q | P"<string>" ]
 *
 *#### Parameters
 *
 * - `Q` - Print the current nozzle configuration
 * - `P"<string>"` - Check printer model name
 *   - `MINI`
 *   - `MK3.5`
 *     - `MK3.5MMU3`
 *   - `MK3.5S`
 *     - `MK3.5SMMU3`
 *   - `MK3.9`
 *     - `MK3.9MMU3`
 *   - `MK3.9S`
 *     - `MK3.9SMMU3`
 *   - `MK4`
 *     - `MK4MMU3`
 *   - `MK4S`
 *     - `MK4SMMU3`
 *   - `XL`
 *   - `iX`
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

    if (parser.seenval('P')) {
        const char *arg = parser.string_arg;
        while (*arg == ' ' || *arg == '\"') {
            arg++;
        }

        const char *arg_end = arg;
        while (std::isalnum(static_cast<unsigned char>(*arg_end)) || *arg_end == '.') {
            arg_end++;
        }

        print_compatibility(setup_gcode_compatibility(PrinterModelInfo::from_id_str(std::string_view(arg, arg_end - arg))));
    }
}

/** @}*/

#endif
