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
 * M862.1: Check nozzle properties
 *
 * ## Parameters
 * - 'Q' - If preset, prints the current HW configuration of a tool T to the serial line
 * - `T` - <number> - specific tool, default to currently active nozzle
 * - 'P' - <number> - expected diameter of the nozzle
 * - 'A' - <0/1> - whether the nozzle should be hardened (the material is abrasive)
 * - 'F' - <0/1> - whether the nozzle should be high-flow
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
