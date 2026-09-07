/**
 * @file
 */
#include "PrusaGcodeSuite.hpp"
#include "Marlin/src/gcode/parser.h"
#include "gcode/gcode.h"
#include "common/printer_model.hpp"
#include <gcode_info.hpp>
#include <raii/scope_guard.hpp>
#include <module/planner.h>
#include <option/has_gcode_compatibility.h>

#include <option/has_chamber_api.h>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
    #include <marlin_stubs/feature/chamber/M141_M191.hpp>
#endif

#ifdef PRINT_CHECKING_Q_CMDS

using namespace buddy;

static void setup_gcode_compatibility(const PrinterModelInfo *gcode_printer) {
    // Failed to identify the printer -> do nothing
    if (!gcode_printer) {
        return;
    }

    gcode.compatibility = PrinterModelInfo::current().gcode_compatibility_report(*gcode_printer);

    #if HAS_CHAMBER_API() && HAS_GCODE_COMPATIBILITY()
    if (gcode.compatibility.chamber_compatibility_mode) {
        FilamentTypeParameters params;

        // Try to somewhat deduce chamber temperature from the loaded filaments from whatever extruder is used
        GCodeInfo::getInstance().for_each_used_extruder([&](GcodeToolIndex, VirtualToolIndex virtual_tool, const GCodeInfo::ExtruderInfo &) {
            params = FilamentType::for_tool_heuristic(virtual_tool).parameters();
        });

        // If the maximum chamber temperature is specified, enqueue cooling
        if (const auto &t = params.chamber_max_temperature; t.has_value() && chamber().current_temperature() > *t) {
            PrusaGcodeSuite::M141_no_parser({ .target_temp = (buddy::Temperature)*t, .wait_for_cooling = true });
        }

        if (const auto &t = params.chamber_min_temperature; t.has_value() && chamber().current_temperature() < *t) {
        #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
            ScopeGuard restore_bed_temp = [temp = thermalManager.degTargetBed()] {
                thermalManager.setTargetBed(temp);
            };

            // C1/C1L does heating using the bed -> set the bed temp to a decently high value
            //
            // C1L runs an additional fan under the bed to distribute the heat.
            // But that's part of the M141 wait for heating thing below.
            thermalManager.setTargetBed(100);

            // Home so that we can set the bed to correct postion
            GcodeSuite::G28_no_parser(true, true, true,
                {
                    .only_if_needed = true,
                    .precise = false, // We don't need to be precisely homed for this
                });

            // Move the bed up to reduce chamber volume
            do_blocking_move_to_z(10);
        #elif PRINTER_IS_PRUSA_XL()
            // XL/XLS: chamber_compatibility_mode is never set for XL-on-XLS compat,
            // so this branch is unreachable. No chamber heating logic needed.
            debug_assert("XL/XLS: chamber_compatibility_mode - should be unreachable");
        #else
            #error Please implement heating logic for this printer
        #endif

            PrusaGcodeSuite::M141_no_parser({ .target_temp = (buddy::Temperature)*t, .wait_for_heating = true });
        }

        chamber().set_target_temperature(params.chamber_target_temperature);
    }
    #endif
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
