/**
 * @file
 * @brief Parsing of M170X g-codes
 */

#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1700: Preheat
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M [ T | W | S | E | B0 ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *       For non-mixing, current extruder if omitted.
 *   - `T-1` - all extruders
 * - `W` - Preheat
 *   - `W0` - preheat no return no cool down
 *   - `W1` - preheat with cool down option
 *   - `W2` - preheat with return option
 *   - `W3` - preheat with cool down and return options - default
 * - `S` - Set filament
 * - `E` - Enforce target temperature
 * - `B0`- Do not preheat bed, default preheat bed
 */
void PrusaGcodeSuite::M1700() {
    const uint8_t preheat = std::min(parser.byteval('W', 3), uint8_t(RetAndCool_t::last_));

    int8_t target_extruder;
    if (parser.seen('T') && parser.intval('T') == -1) {
        target_extruder = -1; // -1 means all extruders
    } else {
        target_extruder = GcodeSuite::get_target_extruder_from_command(); // Get particular extruder or current extruder
        if (target_extruder < 0) {
            return;
        }
    }

    filament_gcodes::M1700_no_parser(RetAndCool_t(preheat), PreheatMode::None, target_extruder,
        parser.boolval('S'), parser.boolval('E'), parser.boolval('B', true));
}

/**
 *### M1701: Autoload
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M1701 [ T | Z | L ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 *        For non-mixing, current extruder if omitted.
 * - `Z` - Move the Z axis by this distance
 * - `L` - Extrude distance for insertion (positive value) (manual reload)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M1701() {
    const bool isL = parser.seen('L');
    const std::optional<float> fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : FILAMENT_CHANGE_FAST_LOAD_LENGTH);
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0) {
        return;
    }

    filament_gcodes::M1701_no_parser(fast_load_length, min_Z_pos, target_extruder);
}

/**
 *### M1600: non-print filament change <a href=" "> </a>
 *
 * Internal GCode
 *
 * not meant to be used during print
 *
 *#### Usage
 *
 *    M1600 [ T | R | U | S | O ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number. Required for mixing extruder.
 * - `R` -  Preheat Return option
 * - `U` - Ask Unload type
 *   - `U0` - return if filament unknown (default)
 *   - `U1` - ask only if filament unknown
 *   - `U2` - always ask
 * - `S"Filament"` - change to filament by name, for example `S"PLA"`
 * - `O<value>` - Color number corresponding to Color, RGB order
 */
void PrusaGcodeSuite::M1600() {
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0) {
        return;
    }

    const FilamentType filament_to_be_loaded = PrusaGcodeSuite::get_filament_type_from_command('S');

    std::optional<Color> color_to_be_loaded = { std::nullopt };
    if (parser.seen('O')) {
        color_to_be_loaded = Color::from_raw(parser.longval('O'));
    }

    const filament_gcodes::AskFilament_t ask_unload = filament_gcodes::AskFilament_t(parser.byteval('U', 0));
    const bool hasReturn = parser.seen('R');

    filament_gcodes::M1600_no_parser(filament_to_be_loaded, target_extruder, hasReturn ? RetAndCool_t::Return : RetAndCool_t::Neither, ask_unload, color_to_be_loaded);
}

/** @}*/
