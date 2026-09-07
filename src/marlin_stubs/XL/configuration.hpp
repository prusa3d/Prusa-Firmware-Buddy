#pragma once
/**
 * @file configuration.hpp
 * @brief non constant definitions from Configuration_XL(_adv).h
 * (Prusa eeprom dependent for at least one Prusa printer type)
 */
#include <cstdint>
#include <cmath>
#include <config_store/store_instance.hpp>

/// Allowed distance in mm between the two homing probes on X/Y.
/// The stall triggers on a full-step boundary, so the window has to accept the probes landing
/// one full step apart and still reject two. The transmission moves 40 mm per motor revolution
/// with the 2GT belts and 40.5 mm with the 1.5GT ones, making a full step 0.2 mm and 0.2025 mm
/// on the 200-step motors - the 2GT window rejects every 1.5GT single-step difference.
inline float axis_home_xy_diff() {
    return config_store().belts_15gt_installed.get() ? 0.25f : 0.2f;
}

// ranges in mm - allowed distance between homing probes for XYZ axes
inline float axis_home_min_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    const float xy = axis_home_xy_diff();
    float arr[] = { -xy, -xy, -0.1f };
    return arr[axis_num];
}

inline float axis_home_max_diff(uint8_t axis_num) {
    if (axis_num >= 3) {
        return NAN;
    }
    const float xy = axis_home_xy_diff();
    float arr[] = { xy, xy, 0.1f };
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
