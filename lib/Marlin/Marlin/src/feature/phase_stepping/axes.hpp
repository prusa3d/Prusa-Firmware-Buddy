#pragma once

#include <cstdint>
#include <core/types.h>

#include "common.hpp"

namespace phase_stepping {

void initialize_axis_motor_params();

/**
 * Return a motor step count for given axis
 **/
int get_motor_steps(AxisEnum motor);

/**
 * Given position, compute coefficient for converting position to motor phase
 **/
int32_t pos_to_phase(AxisEnum axis, float position);

/**
 * Given position, compute step equivalent
 **/
int32_t pos_to_steps(AxisEnum axis, float position);

/**
 * Given position, compute planner msteps equivalent
 **/
int32_t pos_to_msteps(AxisEnum axis, float position);

/**
 * Given position or speed in length unit, return it in revolution units
 **/
float mm_to_rev(AxisEnum axis, float mm);

/**
 * Given a number of revolutions, return it in length units
 **/
float rev_to_mm(AxisEnum axis, float rev);

/**
 * Given motor, report number of phase steps for single µstep
 */
int phase_per_ustep(AxisEnum axis);

} // namespace phase_stepping
