#include "PrusaGcodeSuite.hpp"

#include <logging/log.hpp>
#include <feature/auto_retract/auto_retract.hpp>
#include <gcode/gcode_parser.hpp>
#include <module/planner.h>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(PRUSA_GCODE);

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1705: Autoretract
 *
 *#### Usage
 *
 *    M1705 [N]
 *
 *#### Parameters
 *
 * - `N` - Do not park over the wastebin before ramming.
 */
void M1705() {
    if (std::holds_alternative<NoTool>(PhysicalToolIndex::currently_selected())) {
        log_error(PRUSA_GCODE, "autoretract on invalid tool");
        debug_assert(false);
        return;
    }

    buddy::auto_retract_detail::RetractFromNozzleParams params;
#if HAS_WASTEBIN()
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }
    params.park_over_wastebin = !(parser.option<bool>('N').value_or(false));
#endif
    buddy::auto_retract().maybe_retract_from_nozzle(params);
    planner.synchronize();
}

/** @}*/

} // namespace PrusaGcodeSuite
