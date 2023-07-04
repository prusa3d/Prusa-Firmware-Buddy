
#include "core/serial.h"
#include "gcode/parser.h"
#include "inc/MarlinConfig.h"
#include "PrusaGcodeSuite.hpp"
#include "module/prusa/spool_join.hpp"

#if ENABLED(PRUSA_TOOL_MAPPING)
/**
 * Spool join settings gcode
 *
 * ## Examples
 *
 * - `M864 J A1 B2` - When tool 1 runs out of filament, continue with tool 2
 * - `M864 R` - reset any settings
 * - `M864` - Print current join settings
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
#endif
