
#include "core/serial.h"
#include "gcode/parser.h"
#include "inc/MarlinConfig.h"
#include "PrusaGcodeSuite.hpp"
#include "module/prusa/spool_join.hpp"

#if ENABLED(PRUSA_TOOL_MAPPING)
/** \addtogroup G-Codes
 * @{
 */

/**
 *### M864: Spool join <a href="https://reprap.org/wiki/G-code#M864_Spool_join">M864 Spool join</a>
 *
 * Only MK3.5/S, MK3.9/S, MK4/S with MMU and XL
 *
 *#### Usage
 *
 *    M864 [ J | A | B | R ]
 *
 *#### Parameters
 *
 *
 *#### Examples
 *
 *    M864 J A1 B2 ; When tool 1 runs out of filament, continue with tool 2
 *    M864 R       ; reset any settings
 *    M864         ; Print current join settings
 *
 * Without parameters prints the current Spool join mapping
 */
void PrusaGcodeSuite::M864() {
    if (parser.seen('J') && parser.seen("A") && parser.seen("B")) {
        // map logical tool to physical
        const uint8_t spool_1 = parser.byteval('A');
        const uint8_t spool_2 = parser.byteval('B');

        bool res = spool_join.add_join(spool_1, spool_2);
        if (!res) {
            SERIAL_ERROR_MSG("Invalid join");
        }
    } else if (parser.seen('R')) {
        // reset spool join settings
        spool_join.reset();
    } else {
        // print current spool join settings
        SERIAL_ECHOLN("Spool joins: ");
        for (size_t i = 0; i < spool_join.get_num_joins(); i++) {
            SpoolJoin::join_config_t join = spool_join.get_join_nr(i);
            SERIAL_ECHOLNPAIR("  When tool ", join.spool_1, " runs out, ", join.spool_2, " will continue");
        }
    }
}

/** @}*/

#endif
