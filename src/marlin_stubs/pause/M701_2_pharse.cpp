#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "RAII.hpp"
#include "M70X.hpp"
#include "fs_event_autolock.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "pause_stubbed.hpp"

using namespace filament_gcodes;

/**
 * M701: Load filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  Z<distance> - Move the Z axis by this distance
 *  L<distance> - Extrude distance for insertion (positive value)
 *              - 0 == PURGE
 *  S"Filament" - save filament by name, for example S"PLA". RepRap compatible.
 *  P<mmu>      - MMU index of slot (zero based)
 *  W<value>    - Preheat
 *              - W255 - default without preheat
 *              - W0  - preheat no return no cool down
 *              - W1  - preheat with cool down option
 *              - W2  - preheat with return option
 *              - W3  - preheat with cool down and return options
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    filament_t filament_to_be_loaded = filament_t::NONE;
    const char *text_begin = 0;
    if (parser.seen('S')) {
        text_begin = strchr(parser.string_arg, '"');
        if (text_begin) {
            ++text_begin; // move pointer from '"' to first letter
            const char *text_end = strchr(text_begin, '"');
            if (text_end) {
                filament_t filament = Filaments::FindByName(text_begin, text_end - text_begin);
                if (filament != filament_t::NONE) {
                    filament_to_be_loaded = filament;
                }
            }
        }
    }
    const bool isL = (parser.seen('L') && (!text_begin || strchr(parser.string_arg, 'L') < text_begin));
    const float fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : NAN);
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    const uint8_t preheat = parser.byteval('W', 255);

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    //TODO colision with "PLA" string
    const float mmu_slot = parser.intval('P', -1);

    std::optional<RetAndCool_t> op_preheat = std::nullopt;
    if (preheat <= uint8_t(RetAndCool_t::last_)) {
        op_preheat = RetAndCool_t(preheat);
    }

    M701_no_parser(filament_to_be_loaded, fast_load_length, min_Z_pos, op_preheat, target_extruder, mmu_slot);
}

/**
 * M702: Unload filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, if omitted, current extruder
 *                (or ALL extruders with FILAMENT_UNLOAD_ALL_EXTRUDERS).
 *  Z<distance> - Move the Z axis by this distance
 *  U<distance> - Retract distance for removal (manual reload)
 *  W<value>    - Preheat
 *              - W255 - default without preheat
 *              - W0  - preheat no return no cool down
 *              - W1  - preheat with cool down option
 *              - W2  - preheat with return option
 *              - W3  - preheat with cool down and return options
 *  I           - ask successful unload
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    const float unload_len = parser.seen('U') ? parser.value_axis_units(E_AXIS) : NAN;
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);
    const uint8_t preheat = parser.byteval('W', 255);
    const bool ask_unloaded = parser.seen('I');

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    std::optional<RetAndCool_t> op_preheat = std::nullopt;
    if (preheat <= uint8_t(RetAndCool_t::last_)) {
        op_preheat = RetAndCool_t(preheat);
    }

    M702_no_parser(unload_len, min_Z_pos, op_preheat, target_extruder, ask_unloaded);
}
