/*
 * CoreXY precise homing refinement
 * TODO: @wavexx: Add some documentation
 */

#pragma once
#include "../../core/types.h"

// convert raw AB steps to XY mm
void corexy_ab_to_xy(const xy_long_t &steps, xy_pos_t &mm);

// convert raw AB steps to XY mm, filling others from current state
void corexy_ab_to_xyze(const xy_long_t &steps, xyze_pos_t &mm);

enum class CoreXYCalibrationMode {
    OnDemand, // Allow automatic calibration
    Force, // Force re-calibration
    Disallow // Disallow re-calibration
};

/**
 * @brief Refine home origin precisely on core-XY.
 * @param can_calibrate Allow origin self-calibration to occur
 * @return true on success
 */
bool refine_corexy_origin(CoreXYCalibrationMode mode);
