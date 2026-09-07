/*
 * CoreXY kinematic transforms (AB stepper space <-> XY machine space).
 *
 * These are pure CoreXY kinematics and live in a separate TU so they remain
 * available when homing_corexy.cpp (precise homing refinement) is not compiled in.
 */

#pragma once
#include <core/mtypes.hpp>

// convert raw AB steps to XY mm
MachinePosXY corexy_ab_to_xy(const ab_steps_t &steps);

// convert raw AB steps to XY mm, filling others from current state
void corexy_ab_to_xyze(const ab_steps_t &steps, MachinePosXYZE &mm);
