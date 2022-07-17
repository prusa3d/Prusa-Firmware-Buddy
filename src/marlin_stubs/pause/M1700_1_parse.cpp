/**
 * @file M1700_1_parse.cpp
 * @brief Parsing of M170X g-codes
 */

#include "config_features.h"
#include "../PrusaGcodeSuite.hpp"
#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M70X.hpp"

/**
 * M1700: Preheat
 * not meant to be used during print
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  W<value>    - Preheat
 *              - W0  - preheat no return no cool down
 *              - W1  - preheat with cool down option
 *              - W2  - preheat with return option
 *              - W3  - preheat with cool down and return options - default
 *
 *  S           - Set filament
 *
 *  E           - Enforce target temperature
 */
void PrusaGcodeSuite::M1700() {
    const uint8_t preheat = std::min(parser.byteval('W', 3), uint8_t(RetAndCool_t::last_));

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    filament_gcodes::M1700_no_parser(RetAndCool_t(preheat), target_extruder, parser.boolval('S'), parser.boolval('E'));
}

/**
 * M1701: Autoload
 * not meant to be used during print
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  Z<distance> - Move the Z axis by this distance
 *  L<distance> - Extrude distance for insertion (positive value) (manual reload)
 *
 *  Default values are used for omitted arguments.
 */
void PrusaGcodeSuite::M1701() {
    const bool isL = parser.seen('L');
    const std::optional<float> fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : FILAMENT_CHANGE_FAST_LOAD_LENGTH);
    const float min_Z_pos = parser.linearval('Z', Z_AXIS_LOAD_POS);

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    filament_gcodes::M1701_no_parser(fast_load_length, min_Z_pos, target_extruder);
}

void PrusaGcodeSuite::M1600() {
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    filament_gcodes::M1600_no_parser(target_extruder);
}
