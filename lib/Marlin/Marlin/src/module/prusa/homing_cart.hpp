#pragma once
#include "homing_modus.hpp"
#include "../../core/types.h"

/**
 * \returns offset of current position to calibrated safe home position.
 * This includes HOME_GAP. Works for X and Y axes only.
 */
float calibrated_home_offset(const AxisEnum axis);

/**
 * Provides precise homing for X and Y axes
 * \param axis axis to be homed (cartesian printers only)
 * \param axis_home_dir direction where the home of the axis is
 * \param can_calibrate allows re-calibration if homing is not successful
 * calibration should be disabled for crash recovery, power loss recovery etc.
 * \return probe offset
 */
float home_axis_precise(AxisEnum axis, int axis_home_dir, bool can_calibrate = true, float fr_mm_s = 0);
