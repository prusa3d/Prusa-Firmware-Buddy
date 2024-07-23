/// @file
#pragma once
#include <limits>

#include <device/board.h>
#include "filament_sensor_states.hpp"

namespace FSensorADCEval {

static constexpr int32_t filtered_value_not_ready { std::numeric_limits<int32_t>::min() }; // invalid value of fs_filtered_value
static constexpr int32_t ref_value_not_calibrated { std::numeric_limits<int32_t>::min() }; // invalid value of fs_filtered_value
static constexpr int32_t lower_limit = // value for detecting disconnected sensor
#if (BOARD_IS_XLBUDDY())
    20;
#else
    2000;
#endif

static constexpr int32_t upper_limit =
#if (BOARD_IS_XLBUDDY())
    4096; // this is max value of 12 bit ADC, there is no value that would indicate broken sensor on XL
#else
    2'000'000;
#endif

/**
 * @brief Evaluate state of filament sensor
 * @param filtered_value current filtered value from ADC
 * @param fs_ref_nins_value Reference value with filament NOT inserted
 * @param fs_ref_ins_value Reference value with filament inserted
 * @param fs_value_span configured span of fsensor
 */
FilamentSensorState evaluate_state(int32_t filtered_value, int32_t fs_ref_nins_value, int32_t fs_ref_ins_value, int32_t fs_value_span);

} // namespace FSensorADCEval
