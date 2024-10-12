
#include "core/serial.h"
#include "gcode/parser.h"
#include "inc/MarlinConfig.h"
#include "PrusaGcodeSuite.hpp"
#include "module/prusa/tool_mapper.hpp"

#if ENABLED(PRUSA_TOOL_MAPPING)

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M863: Tool remapping <a href="https://reprap.org/wiki/G-code#M863_Tool_remapping">M863 Tool remapping</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S with MMU and XL
 *
 *#### Usage
 *
 *    M863 [ M | L | P | E | R ]
 *
 *#### Parameters
 *
 * - `M` - Map needs `P' and `L`
 *   - `P` - Physical tool
 *   - `L` - Logical tool
 * - `E` - Set
 *    - `1` - enable
 *    - `0` - disable
 * - `R` - Reset to default
 *
 *#### Examples
 *
 *    M863 M P0 L1 ; Use tool 0 while in gcode tool 1 is selected
 *    M863 E1      ; Enable tool remapping
 *    M863 R       ; Reset tool remapping
 *    M863         ; Print current tool mapping
 *
 * Without parameters prints the current Tool mapping
 */
void PrusaGcodeSuite::M863() {
    if (parser.seen('M') && parser.seen("P") && parser.seen("L")) {
        // map logical tool to physical
        const uint8_t logical = parser.byteval('L');
        const uint8_t physical = parser.byteval('P');

        bool res = tool_mapper.set_mapping(logical, physical);
        if (!res) {
            SERIAL_ERROR_MSG("Invalid mapping");
        }
    } else if (parser.seen('E')) {
        // enable/disable tool mapping
        tool_mapper.set_enable(parser.boolval('E'));
    } else if (parser.seen('R')) {
        // reset tool mapping to default
        tool_mapper.reset();
    } else {
        // print current tool mapping settings
        SERIAL_ECHOLN("Tool mapping: ");
        for (size_t i = 0; i < EXTRUDERS; i++) {
            SERIAL_ECHOPAIR("  Tool ", i, " -> ");
            const uint8_t to = tool_mapper.to_physical(i, true);
            if (to == tool_mapper.NO_TOOL_MAPPED) {
                SERIAL_ECHO("<none>");
            } else {
                SERIAL_ECHO(to);
            }

            SERIAL_EOL();
        }

        SERIAL_ECHOLNPAIR("Enabled: ", tool_mapper.is_enabled());
    }
}

/** @}*/

#endif
