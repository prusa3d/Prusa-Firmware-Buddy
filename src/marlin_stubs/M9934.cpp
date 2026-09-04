#include "PrusaGcodeSuite.hpp"

#include <multi_filament_change.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9934: Multi filament change
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    N9934 (Base64-encoded filament change config)
 *
 */
void PrusaGcodeSuite::M9934() {
    GCodeBasicParser parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

    static_assert(multi_filament_change::gcode_command == GCodeCommand { .letter = 'M', .codenum = 9934 });

    const auto config = multi_filament_change::Config::from_gcode(parser);
    if (!config) {
        return;
    }

    config->execute();
}
/** @}*/
