#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "RAII.hpp"
#include "M70X.hpp"
#include "fs_event_autolock.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/feature/prusa/e-stall_detector.h"
#include "pause_stubbed.hpp"
#include "pause_settings.hpp"

using namespace filament_gcodes;

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M701: Load filament <a href="https://reprap.org/wiki/G-code#M701:_Load_filament">M701: Load filament</a>
 *
 *#### Usage
 *
 *    M701 [ T | Z | L | S | P | W | O | R ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number
 * - `Z` - Move the Z axis by this distance
 * - `L` - Extrude distance for insertion (positive value)
 *   - `0` - PURGE
 * - `S"Filament"` - save filament by name, for example S"PLA". RepRap compatible.
 * - `P<mmu>` - MMU index of slot (zero based)
 * - `W<value>` - Preheat
 *   - `W255` - default without preheat
 *   - `W0` - preheat no return no cool down
 *   - `W1` - preheat with cool down option
 *   - `W2` - preheat with return option
 *   - `W3` - preheat with cool down and return options
 * - `O<value>` - Color number corresponding to Color, RGB order
 * - `R` - resume print if paused
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const FilamentType filament_to_be_loaded = p.option<FilamentType>('S').value_or(NoFilamentType());
    std::optional<Color> color_to_be_loaded = p.option<Color>('O');
    const std::optional<float> fast_load_length = p.option<float>('L').transform(fabsf);
    const float min_Z_pos = p.option<float>('Z').value_or(Z_AXIS_LOAD_POS);
    const auto op_preheat = p.option<RetAndCool_t>('W', std::to_underlying(RetAndCool_t::last_) + 1);

    const int8_t target_extruder = PrusaGcodeSuite::get_target_extruder_from_command(p);
    if (target_extruder < 0) {
        return;
    }

    const int8_t mmu_slot = p.option<int8_t>('P').value_or(-1);
    const ResumePrint_t resume_print = static_cast<ResumePrint_t>(p.option<bool>('R').value_or(false));

    M701_no_parser(filament_to_be_loaded, fast_load_length, min_Z_pos, op_preheat, target_extruder, mmu_slot, color_to_be_loaded, resume_print);
}

/**
 *### M702: Unload filament <a href="https://reprap.org/wiki/G-code#M702:_Unload_filament">M702: Unload filament</a>
 *
 *#### Usage
 *
 *    M702 [ T | Z | U | W | I ]
 *
 *#### Parameters
 *
 * - `T` - Extruder number
 * - `Z` - Move the Z axis by this distance
 * - `U` - Retract distance for removal (manual reload)
 * - `W` - Preheat
 *   - `255` - default without preheat
 *   - `0` - preheat no return no cool down
 *   - `1` - preheat with cool down option
 *   - `2` - preheat with return option
 *   - `3` - preheat with cool down and return options
 * - `I` - ask successful unload
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    const std::optional<float> unload_len = parser.seen('U') ? std::optional<float>(parser.value_axis_units(E_AXIS)) : std::nullopt;
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    const uint8_t preheat = parser.byteval('W', 255);
    const bool ask_unloaded = parser.seen('I');

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0) {
        return;
    }

    std::optional<RetAndCool_t> op_preheat = std::nullopt;
    if (preheat <= uint8_t(RetAndCool_t::last_)) {
        op_preheat = RetAndCool_t(preheat);
    }

    M702_no_parser(unload_len, min_Z_pos, op_preheat, target_extruder, ask_unloaded);
}
/** @}*/
