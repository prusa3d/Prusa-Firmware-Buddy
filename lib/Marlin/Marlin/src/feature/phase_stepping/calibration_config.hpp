#pragma once

#include <printers.h>

#include <optional>
#include <array>
#include <bsod/bsod.h>

namespace phase_stepping {

struct AxisCalibrationConfig {
    std::tuple<float, float> speed_range;
    std::bitset<opts::CORRECTION_HARMONICS> enabled_harmonics;

    float max_movement_revs = 5.f;
    float fine_movement_duration = 2.f;

    float peak_speed_shift = 0.9f;
    /// If set, skips the speed sweep and uses this as the speed.
    ///
    /// This is speed for H1, others are derived appropriately.
    std::optional<float> speed_override = std::nullopt;

    float min_magnitude = 0.008f;
    float max_magnitude = 0.4f;
    float magnitude_quotient = 2.f;

    float analysis_window_size_seconds = 0.1f;
    int analysis_window_periods = 10;
    int speed_sweep_bins = 400;
    int param_sweep_bins = 400;
};

#if PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_COREONE()
// 400-step motors
static inline constexpr const AxisCalibrationConfig xy_axis_calibration_config {
    .speed_range = { 0.2f, 4.f },
    .enabled_harmonics = 0b1010,
};
#elif PRINTER_IS_PRUSA_XL()
// 200-step motors
static inline constexpr const AxisCalibrationConfig xy_axis_calibration_config {
    // Unused due to speed_override, but must be set to make compiler happy.
    .speed_range = { 1.f, 5.f },
    .enabled_harmonics = 0b1010,
    .speed_override = 6.0,
    .min_magnitude = 0.016f,
    .magnitude_quotient = 1.4f,
};
#elif PRINTER_IS_PRUSA_iX()
static inline constexpr const AxisCalibrationConfig xy_axis_calibration_config {
    .speed_range = { 0.1f, 3.f },
    .enabled_harmonics = 0b1010,
    .magnitude_quotient = 1.4f
};
#elif PRINTER_IS_PRUSA_COREONEL()
static inline constexpr const AxisCalibrationConfig xy_axis_calibration_config {
    .speed_range = { 0.2f, 3.f },
    .enabled_harmonics = 0b1010,
    .magnitude_quotient = 1.4f
};
#else
    #error
#endif

inline const AxisCalibrationConfig &get_calibration_config(AxisEnum axis) {
    switch (axis) {
    case AxisEnum::X_AXIS:
    case AxisEnum::Y_AXIS:
        return xy_axis_calibration_config;
    default:
        bsod("Unsupported axis");
    }
}

}; // namespace phase_stepping
