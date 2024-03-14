/**
 * @file screen_menu_move_utils.hpp
 * @brief Utility functions for interactive axis jogging
 */
#pragma once

#include "core/types.h" // for AxisEnum

/**
 * @brief Jog an axis to the requested target position
 *
 * Enqueue the minimal amount of moves possible on the requested axis to move towards the target
 * position smoothly while allowing rapid changes in the target position.
 *
 * This functions needs to called in a loop within the frontend, possibily updating target (which is
 * usually connected to the encoder position).
 *
 * Feedrate is determined by the MenuVars::GetManualFeedrate() for the axis.
 *
 * @param position Input/output enqueued position of the axis
 * @param target Target position of the axis
 * @param axis Axis to jog
 */
void jog_axis(float &position, const float target, const AxisEnum axis);
