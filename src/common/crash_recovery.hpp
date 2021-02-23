/// crash_recovery.hpp
#pragma once

#include "../../Marlin/src/core/types.h"
#include "../../Marlin/src/module/planner.h"
#include "guitypes.h"

/// Like Planner::quick_stop
/// Disable stepper ISR before calling
void crash_quick_stop();

void restore_planner_after_crash(abce_long_t &machine);

/// Plans movement to the end of axis
void homing_start(AxisEnum axis, const bool positive_dir);

/// Call once the axis is at the end
/// \param positive_dir to the same value as in homing_start();
/// \param reset_position resets machine position to the homing position (MAX/MIN_POS)
void homing_finish(AxisEnum axis, const bool positive_dir, const bool reset_position);

void homing_finish_axis(AxisEnum axis);
