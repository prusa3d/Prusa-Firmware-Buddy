/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include <Marlin/src/gcode/parser.h>
#include <Marlin/src/module/motion.h>
#include <config_store/store_instance.hpp>
#include <utils/string_builder.hpp>

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.1: Check nozzle properties <a href="https://reprap.org/wiki/G-code#M862.1:_Check_nozzle_diameter">M862.1: Check nozzle diameter</a>
 *
 *#### Usage
 *
 *    M862.1 [ Q | T | P | A | F ]
 *
 *#### Parameters
 *
 * - `Q` - Print the current nozzle configuration
 * - `T` - Target extruder
 * - `P` - Check diameter of the nozzle
 * - `A` - Abrasive resistent / hardened nozzle
 * - `F` - High-Flow nozzle
 */
void PrusaGcodeSuite::M862_1() {
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    // Get for which tool
    #if HAS_TOOLCHANGER()
    const uint8_t default_tool = active_extruder;
    #else /*HAS_TOOLCHANGER()*/
    const uint8_t default_tool = 0;
    #endif /*HAS_TOOLCHANGER()*/

    SERIAL_ECHO_START();
    uint8_t tool = parser.byteval('T', default_tool);
    if (tool >= config_store_ns::max_tool_count) {
        SERIAL_ECHOPAIR("Tool ", tool, " isn't available\n");
        return;
    }
    auto nozzle_diameter = static_cast<double>(config_store().get_nozzle_diameter(tool));

    if (parser.boolval('Q')) {
        // Fetch diameter from EEPROM
        ArrayStringBuilder<64> sb;
        sb.append_printf("  M862.1 T%u P%.2f A%i F%i", tool, nozzle_diameter, config_store().nozzle_is_hardened.get().test(tool), config_store().nozzle_is_high_flow.get().test(tool));
        SERIAL_ECHO(sb.str());
    }

    if (parser.seenval('P')) {
        const double requested_nozzle_diameter = (double)parser.floatval('P');
        if (requested_nozzle_diameter < 0) {
            SERIAL_ECHO("Requested nozzle diameter cannot be negative.");
        } else if (requested_nozzle_diameter != nozzle_diameter) {
            ArrayStringBuilder<71> sb;
            sb.append_printf("Current nozzle diameter %.2f doesn't match requested diameter of %.2f.", nozzle_diameter, requested_nozzle_diameter);
            SERIAL_ECHO(sb.str());
        } else {
            SERIAL_ECHO("Requested nozzle diameter matches current nozzle diameter.");
        }
    }
    SERIAL_EOL();
}

/** @}*/

#endif
