/**
 * @file
 */
#include "../../../inc/MarlinConfig.h"

#include "../../gcode.h"
#include "../../../module/stepper.h"
#include "../../../feature/pressure_advance/pressure_advance_config.hpp"

static void dump_current_config() {
    if (const pressure_advance::Config &config = pressure_advance::get_axis_e_config(); config.pressure_advance <= 0.) {
        SERIAL_ECHO_MSG("Pressure advance: disabled");
    } else {
        std::array<char, 128> buff;

        snprintf(buff.data(), buff.size(), "Pressure advance: %f", config.pressure_advance);
        SERIAL_ECHO_START();
        SERIAL_ECHOLN(buff.data());

        snprintf(buff.data(), buff.size(), "Pressure advance smooth time: %f", config.smooth_time);
        SERIAL_ECHO_START();
        SERIAL_ECHOLN(buff.data());
    }
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M572: Set or report extruder pressure advance <a href="https://reprap.org/wiki/G-code#M572:_Set_or_report_extruder_pressure_advance">M572: Set or report extruder pressure advance</a>
 *
 *
 *#### Usage
 *
 *    M [ D | S | W ]
 *
 *#### Parameters
 *
 *
 * - `D` - Set the extruder number
 * - `S` - Set the pressure advance value. Range is 0. to 1.0 seconds. If zero the pressure advance is disabled
 * - `W` - Set a time range in seconds used for calculating the average extruder velocity for pressure advance. Range between 0. and 0.2 Default value is 0.04
 *
 * Without parameters prints the current pressure advance settings.
 */
void GcodeSuite::M572() {
    const pressure_advance::Config &pa_config = pressure_advance::get_axis_e_config();
    float pressure_advance = pa_config.pressure_advance;
    float smooth_time = pa_config.smooth_time;

    const bool seen_d = parser.seen('D');
    if (seen_d) {
        SERIAL_ECHO_MSG("?Extruder number is not yet supported");
    }

    const bool seen_s = parser.seen('S');
    if (seen_s) {
        const float s = parser.value_float();
        if (WITHIN(s, 0.f, 10.f)) {
            pressure_advance = s;
        } else {
            SERIAL_ECHO_MSG("?Pressure advance (S) value out of range (0-10)");
        }
    }

    const bool seen_w = parser.seen('W');
    if (seen_w) {
        const float w = parser.value_float();
        if (WITHIN(w, 0.f, 0.2f)) {
            smooth_time = w;
        } else {
            SERIAL_ECHO_MSG("?Pressure advance smooth time (W) value out of range (0-0.2)");
        }
    }

    if (!seen_d && !seen_s && !seen_w) {
        dump_current_config();
        return;
    }

    M572_internal(pressure_advance, smooth_time);
}

/** @}*/

void GcodeSuite::M572_internal(float pressure_advance, float smooth_time) {
    const pressure_advance::Config new_axis_e_config = { .pressure_advance = pressure_advance, .smooth_time = smooth_time };
    if (pressure_advance::get_axis_e_config() != new_axis_e_config) {
        // For now, we must ensure that all queues are empty before changing pressure advance parameters.
        // But later, it could be possible to wait just for block and move quests.
        planner.synchronize();
        if (!planner.draining()) {
            // Only set configuration when the current command isn't aborted
            pressure_advance::set_axis_e_config(new_axis_e_config);
        }
    }
}
