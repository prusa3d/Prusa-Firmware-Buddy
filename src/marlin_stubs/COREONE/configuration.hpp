#pragma once
/**
 * @file configuration.hpp
 * @brief non constant definitions from Configuration_COREONE(_adv).h
 * (Prusa eeprom dependent for at least one Prusa printer type)
 */
#include <cstdint>
#include <cmath>

// ranges in mm - allowed distance between homing probes for XYZ axes
inline constexpr float axis_home_min_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    float arr[] = { -1, -1, -0.1 };
    return arr[axis_num];
}

inline constexpr float axis_home_max_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    float arr[] = { 1, 1, 0.1 };
    return arr[axis_num];
}

inline constexpr float axis_home_invert_min_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    float arr[] = { -1, -1, -1 };
    return arr[axis_num];
}

inline constexpr float axis_home_invert_max_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    float arr[] = { 1, 1, 1 };
    return arr[axis_num];
}
