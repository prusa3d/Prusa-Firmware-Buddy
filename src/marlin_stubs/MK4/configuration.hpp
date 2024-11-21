/**
 * @file configuration.hpp
 * @brief non constant definitions from Configuration_MK4(_adv).h
 * (Prusa eeprom dependent for at least one Prusa printer type)
 */
#pragma once

#include <cstdint>
#include <cmath>

#include <Marlin/src/core/types.h>

// ranges in mm - allowed distance between homing probes for XYZ axes
constexpr float axis_home_max_diff_xy_mk4 = 0.1F;
constexpr float axis_home_min_diff_xy_mk4 = -0.1F;

constexpr float axis_home_max_diff_xy_mk3_9 = 0.2F;
constexpr float axis_home_min_diff_xy_mk3_9 = -0.2F;

constexpr float axis_home_max_diff_z = 0.1F;
constexpr float axis_home_min_diff_z = -0.1F;

float axis_home_min_diff(uint8_t axis_num);

float axis_home_max_diff(uint8_t axis_num);

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
