/**
 * @file
 * @brief Parsing of M170X g-codes
 */

#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"
#include <utils/variant_utils.hpp>

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
 *    M1700 [ T | W | S | E | B0 | C0 | H0 ]
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
 * - `B0` - Do not preheat the bed
 * - `C0` - Do not set chamber temperature
 * - `H0` - Do not set heatbreak temperature
 */
void PrusaGcodeSuite::M1700() {
    const uint8_t preheat = std::min(parser.byteval('W', 3), uint8_t(RetAndCool_t::last_));

    std::variant<VirtualToolIndex, AllTools> tool = AllTools {};
    if (parser.intval('T') == -1) {
        tool = AllTools {}; // -1 means all extruders
    } else {
        const std::optional<VirtualToolIndex> vt = stdext::get_optional<VirtualToolIndex>(GcodeSuite::get_target_virtual_from_command()); // Get particular extruder or current extruder
        if (vt.has_value()) {
            tool = *vt;
        }
    }

    filament_gcodes::M1700_preheat(filament_gcodes::M1700Args {
        .preheat = RetAndCool_t(preheat),
        .mode = PreheatMode::preheat,
        .tool = tool,
        .save = parser.boolval('S'),
        .enforce_target_temp = parser.boolval('E'),
        .preheat_bed = parser.boolval('B', true),
#if HAS_CHAMBER_API()
        .preheat_chamber = parser.boolval('C', true),
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
        .set_heatbreak = parser.boolval('H', true),
#endif
    });
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
 * - `Z` - Minimum Z park position
 * - `L` - Extrude distance for insertion (positive value) (manual reload)
 *
 * Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M1701() {
    const bool isL = parser.seen('L');
    const std::optional<float> fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : FILAMENT_CHANGE_FAST_LOAD_LENGTH);
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);

    const std::optional<VirtualToolIndex> virtual_tool = stdext::get_optional<VirtualToolIndex>(GcodeSuite::get_target_virtual_from_command());
    if (!virtual_tool.has_value()) {
        return;
    }

    filament_gcodes::M1701_autoload(fast_load_length, min_Z_pos, virtual_tool->to_raw());
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
 * - `P` - If set, the parameter 'T' is interpreted as a VirtualToolIndex (tool mapping is not applied)
 */
void PrusaGcodeSuite::M1600() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const std::optional<VirtualToolIndex> virtual_tool = stdext::get_optional<VirtualToolIndex>(PrusaGcodeSuite::get_target_virtual_from_command_p(p));
    if (!virtual_tool.has_value()) {
        return;
    }

    const FilamentType filament_to_be_loaded = p.option<FilamentType>('S').value_or(NoFilamentType());
    std::optional<Color> color_to_be_loaded = p.option<Color>('O');
    const filament_gcodes::AskFilament_t ask_unload = filament_gcodes::AskFilament_t(p.option<int>('U').value_or(0));
    const bool hasReturn = p.option<bool>('R').value_or(false);

    filament_gcodes::M1600_change_filament(filament_to_be_loaded, *virtual_tool, hasReturn ? RetAndCool_t::Return : RetAndCool_t::Neither, ask_unload, color_to_be_loaded);
}

/** @}*/
