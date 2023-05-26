/**
 * Based on Marlin 3D Printer Firmware
 * Copyright (c) 2022 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 */
#include "../../../inc/MarlinConfig.h"

#include "../../gcode.h"
#include "../../../feature/input_shaper/input_shaper.h"
#include "../../../module/stepper.h"

static input_shaper_pulses_t get_input_shaper(const input_shaper::Type input_shaper_type, const float input_shaper_frequency, const float damping_ratio, const float vibration_reduction) {
    switch (input_shaper_type) {
    case input_shaper::Type::zv:
        return create_zv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::zvd:
        return create_zvd_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::mzv:
        return create_mzv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    case input_shaper::Type::ei:
        return create_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_2hump:
        return create_2hump_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    case input_shaper::Type::ei_3hump:
        return create_3hump_ei_input_shaper_pulses(input_shaper_frequency, damping_ratio, vibration_reduction);
    default:
        return create_zv_input_shaper_pulses(input_shaper_frequency, damping_ratio);
    }
}

/**
* @brief Set parameters for input shapers.
*
* - D<ratio>     Set the input shaper damping ratio. If axes (X, Y, etc.) are not specified, set it for all axes. Default value is 0.1.
* - F<frequency> Set the input shaper frequency. If axes (X, Y, etc.) are not specified, set it for all axes. Default value is 0Hz - It means that the input shaper is disabled.
* - T[map]       Set the input shaper type, 0:ZV, 1:ZVD, 2:MZV, 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 0:ZV.
* - R<reduction> Set the input shaper vibration reduction. This parameter is used just for 3:EI, 4:2HUMP_EI, and 5:3HUMP_EI. Default value is 20.
* - X<1>         Set the input shaper parameters only for the X axis.
* - Y<1>         Set the input shaper parameters only for the Y axis.
*/
void GcodeSuite::M593() {
    const bool seen_x = parser.seen('X');
    const bool seen_y = parser.seen('Y');

    float damping_ratio = 0.1;
    float frequency = 50.;
    float vibration_reduction = 20.;
    input_shaper::Type type = input_shaper::Type::zv;

    if (parser.seen('D')) {
        const float dr = parser.value_float();
        if (WITHIN(dr, 0., 1.))
            damping_ratio = dr;
        else
            SERIAL_ECHO_MSG("?Damping ratio (D) value out of range (0-1)");
    }

    if (parser.seen('F')) {
        const float f = parser.value_float();
        if (f >= 0)
            frequency = f;
        else
            SERIAL_ECHO_MSG("?Frequency (X) must be greater or equal to 0");
    }

    if (parser.seen('T')) {
        const int t = parser.value_int();
        if (WITHIN(t, 0, 5))
            type = static_cast<input_shaper::Type>(t);
        else
            SERIAL_ECHO_MSG("?Invalid type of input shaper (T)");
    }

    if (parser.seen('R')) {
        const float vr = parser.value_float();
        if (vr > 0)
            vibration_reduction = vr;
        else
            SERIAL_ECHO_MSG("?Vibration reduction (X) must be greater than 0");
    }
    input_shaper::set(seen_x, seen_y, damping_ratio, frequency, vibration_reduction, type);
}

namespace input_shaper {

void set(bool seen_x, bool seen_y, float damping_ratio, float frequency, float vibration_reduction, input_shaper::Type type) {
    // For now, we must ensure that all queues are empty before changing input shapers parameters.
    // But later, it could be possible to wait just for block and move quests.
    planner.synchronize();

    if (seen_x || !seen_y) {
        if (frequency > 0) {
            InputShaper::is_pulses_x = get_input_shaper(type, frequency, damping_ratio, vibration_reduction);
            PreciseStepping::step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_X;
        } else
            PreciseStepping::step_generator_types &= ~INPUT_SHAPER_STEP_GENERATOR_X;
        InputShaper::x_frequency = frequency; // Store for GUI
        InputShaper::x_type = type;           // Store for GUI
    }

    if (seen_y || !seen_x) {
        if (frequency > 0) {
            InputShaper::is_pulses_y = get_input_shaper(type, frequency, damping_ratio, vibration_reduction);
            PreciseStepping::step_generator_types |= INPUT_SHAPER_STEP_GENERATOR_Y;
        } else
            PreciseStepping::step_generator_types &= ~INPUT_SHAPER_STEP_GENERATOR_Y;
        InputShaper::y_frequency = frequency; // Store for GUI
        InputShaper::y_type = type;           // Store for GUI
    }
}
} //namespace input_shaper
