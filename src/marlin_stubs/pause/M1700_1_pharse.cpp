/**
 * @file M1700_1_pharse.cpp
 * @brief Pharsing of M170X g-codes
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
 */
void PrusaGcodeSuite::M1700() {
    const float preheat = parser.linearval('W', 3);

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    filament_gcodes::M1700_no_parser(PreheatData::GetRetAndCool(preheat), target_extruder);
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
    const float fast_load_length = std::abs(isL ? parser.value_axis_units(E_AXIS) : NAN);
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
