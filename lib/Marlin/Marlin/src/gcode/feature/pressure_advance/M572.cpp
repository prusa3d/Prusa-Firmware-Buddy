/**
 * @file
 */
#include "../../../inc/MarlinConfig.h"

#include "../../gcode.h"
#include "../../../module/stepper.h"
#include "../../../feature/pressure_advance/pressure_advance_config.hpp"

/**
 * @brief Set parameters for pressure advance.
 *
 * - D<value>     Set the extruder number.
 * - S<value>     Set the pressure advance value. If zero the pressure advance is disabled.
 * - W<time>      Set a time range in seconds used for calculating the average extruder velocity for pressure advance. Default value is 0.04.
 */
void GcodeSuite::M572() {
    float pressure_advance = 0.;
    float smooth_time = 0.04;

    if (parser.seen('D')) {
        SERIAL_ECHO_MSG("?Extruder number is not yet supported");
    }

    if (parser.seen('S')) {
        const float s = parser.value_float();
        if (WITHIN(s, 0., 1.))
            pressure_advance = s;
        else
            SERIAL_ECHO_MSG("?Pressure advance (S) value out of range (0-1)");
    }

    if (parser.seen('W')) {
        const float w = parser.value_float();
        if (WITHIN(w, 0., 0.2))
            smooth_time = w;
        else
            SERIAL_ECHO_MSG("?Pressure advance smooth time (W) value out of range (0-0.2)");
    }

    M572_internal(pressure_advance, smooth_time);
}

void GcodeSuite::M572_internal(float pressure_advance, float smooth_time) {
    // For now, we must ensure that all queues are empty before changing pressure advance parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();

    pressure_advance::set_axis_e_config({
        .pressure_advance = pressure_advance,
        .smooth_time = smooth_time,
    });
}
