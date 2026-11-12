#pragma once

#include <printers.h>
#include <option/has_automatic_belt_tuning.h>

namespace manual_belt_tuning {

// the limit of valid frequency diff in Hz
constexpr uint16_t freq_diff_limit = 8;

// frequency range
constexpr uint16_t freq_min = 60;
constexpr uint16_t freq_max = 130;

// acceleration range
constexpr uint16_t accel_min = 7000;
constexpr uint16_t accel_max = 10500;

// 1m length belt waight
// Note iX has GT1.5 belts, but the weight difference was deemed insignificant
constexpr float nominal_weight_kg_m = 0.007569f;

#if PRINTER_IS_PRUSA_COREONE()

// constants for calculating how many revolutions to do for any frequency differences
// this constants represents average frequency change of the belt per one revolution of its own screw
constexpr uint16_t belt_hz_per_rev = 15;
// this constants represents average frequency change of the belt per one revolution of second belt screw
constexpr uint16_t belt_hz_per_rev2 = 12;

    #if HAS_INDX()

// optimal belt tension for the printer
constexpr uint16_t tension_optimal_N = 21;

// top belt length (vibrating part)
constexpr float length_top_belt = 0.270175f;
// bottom belt length (vibrating part)
constexpr float length_bottom_belt = 0.2855f;

constexpr int16_t calib_position_x = 251;
constexpr int16_t calib_position_y = 30;

    #else

// optimal belt tension for the printer
constexpr uint16_t tension_optimal_N = 19;

// avg belt length (vibrating part)
constexpr float length_belt = 0.267f;
// top belt length (vibrating part)
constexpr float length_top_belt = length_belt - 0.005f;
// bottom belt length (vibrating part)
constexpr float length_bottom_belt = length_belt + 0.005f;

constexpr int16_t calib_position_x = 240;
constexpr int16_t calib_position_y = -8;

    #endif

#elif PRINTER_IS_PRUSA_COREONEL()

// top belt length (vibrating part) 318 - 6.5 = 311.5mm
constexpr float length_top_belt = 0.3115f;
// bottom belt length (vibrating part) 318 + 6.5 = 324.5mm
constexpr float length_bottom_belt = 0.3245f;

constexpr int16_t calib_position_x = 294;
constexpr int16_t calib_position_y = 0;

// constants for calculating how many revolutions to do for any frequency differences
// this constants represents average frequency change of the belt per one revolution of its own screw
constexpr uint16_t belt_hz_per_rev = 12;
// this constants represents average frequency change of the belt per one revolution of second belt screw
constexpr uint16_t belt_hz_per_rev2 = 10;

// optimal belt tension for the printer
constexpr uint16_t tension_optimal_N = 25;

#elif PRINTER_IS_PRUSA_iX()

// avg belt length (vibrating part)
constexpr float length_belt = 0.3f;
// top belt length (vibrating part)
constexpr float length_top_belt = length_belt - 0.0065f;
// bottom belt length (vibrating part)
constexpr float length_bottom_belt = length_belt + 0.0065f;

constexpr int16_t calib_position_x = 257;
constexpr int16_t calib_position_y = 8;

// constants for calculating how many revolutions to do for any frequency differences
// this constants represents average frequency change of the belt per one revolution of its own screw
constexpr uint16_t belt_hz_per_rev = 12;
// this constants represents average frequency change of the belt per one revolution of second belt screw
constexpr uint16_t belt_hz_per_rev2 = 10;

// optimal belt tension for the printer
constexpr uint16_t tension_optimal_N = 18;

#else
    #error
#endif

// Calculates belt tension from their resonant frequency.
// returns tension (in Newtons)
// Formula taken from http://www.hyperphysics.gsu.edu/hbase/Waves/string.html
constexpr float freq_to_tension(float frequency_hz, float length_m) {
    return 4 * nominal_weight_kg_m * length_m * length_m * frequency_hz * frequency_hz;
}

// Calculates resonant frequency from the tension
// returns resonant frequency (in Hz)
constexpr float tension_to_freq(uint16_t tension_n, float length_m) {
    return sqrt(tension_n / (4 * nominal_weight_kg_m * length_m * length_m));
}

// calculate acceleration for desired frequency
constexpr float calc_accel(float freq) {
    // returns acceleration
    return accel_min + (freq - freq_min) * (accel_max - accel_min) / (freq_max - freq_min);
}

// calculate how many revolutions should user rotate the screws
constexpr float calc_revs_from_freq(float df1, float df2, float k1, float k2) {
    // df1 - first belt frequency difference (actual - optimal)
    // df2 - second belt frequency difference (actual - optimal)
    // k1 - first belt frequency constant (hz/revs)
    // k2 - second belt frequency constant (hz/revs)
    // returns requiered revs for first belt
    return ((df2 * k2) - (df1 * k1)) / ((k2 * k2) - (k1 * k1));
}

constexpr float floor_to_half(float val) {
    return static_cast<float>(static_cast<int>(val * 2)) / 2.0f;
}

#if HAS_AUTOMATIC_BELT_TUNING()

// Automatic measurement with the head accelerometer. The values below come from a single printer.

// frequency change of a belt per one revolution of its own screw, and of the other belt's screw
constexpr float automatic_hz_per_rev = 27.f;
constexpr float automatic_hz_per_rev2 = 17.f;

// accepted distance of a belt from its optimal frequency
constexpr float automatic_freq_tolerance = 3.f;

// the most to ask for at once on one screw, in eighths of a turn; the belts get measured again anyway
constexpr int8_t automatic_max_turn_eights = 2;

// how far above its frequency the lower belt usually collapses when the frequency is swept up
constexpr float automatic_collapse_distance = 9.f;

// adjustments after which the user gets asked whether to go on, when the belts are still out of the tolerance
constexpr int automatic_adjustment_rounds = 5;

// the second place the belts get measured at, relative to the calibration position
constexpr float automatic_second_position_x_offset = -20.f;

#endif

// optimal frequency for top belt (in some cases lower belt has higher freq and optimal frequencies are switched)
constexpr float higher_freq_belt_optimal = floor_to_half(tension_to_freq(tension_optimal_N, length_top_belt));
// optimal frequency for bottom belt (in some cases lower belt has higher freq and optimal frequencies are switched)
constexpr float lower_freq_belt_optimal = floor_to_half(tension_to_freq(tension_optimal_N, length_bottom_belt));

// outside measurements are considered to have x-gantry misalignment
constexpr uint16_t freq_result_min = static_cast<uint16_t>(lower_freq_belt_optimal - 3);
constexpr uint16_t freq_result_max = static_cast<uint16_t>(higher_freq_belt_optimal + 3);

}; // namespace manual_belt_tuning
