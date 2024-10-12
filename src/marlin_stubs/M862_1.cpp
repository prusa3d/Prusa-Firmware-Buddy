/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include <Marlin/src/gcode/parser.h>
#include <Marlin/src/module/motion.h>
#include <config_store/store_instance.hpp>
#include <str_utils.hpp>

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
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (!parser.boolval('Q')) {
        return;
    }

    // Get for which tool
    #if HAS_TOOLCHANGER()
    const uint8_t default_tool = active_extruder;
    #else /*HAS_TOOLCHANGER()*/
    const uint8_t default_tool = 0;
    #endif /*HAS_TOOLCHANGER()*/
    uint8_t tool = parser.byteval('T', default_tool);

    // Fetch diameter from EEPROM
    if (tool < config_store_ns::max_tool_count) {
        SERIAL_ECHO_START();
        ArrayStringBuilder<64> sb;
        sb.append_printf("  M862.1 T%u P%.2f A%i F%i", tool, static_cast<double>(config_store().get_nozzle_diameter(tool)), config_store().nozzle_is_hardened.get().test(tool), config_store().nozzle_is_high_flow.get().test(tool));
        SERIAL_ECHO(sb.str());
        SERIAL_EOL();
    }
}

/** @}*/

#endif
