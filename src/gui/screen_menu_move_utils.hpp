/**
 * @file screen_menu_move_utils.hpp
 * @brief Utility functions for interactive axis jogging
 */
#pragma once

#include "core/types.h" // for AxisEnum
#include <span>

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

/**
 * @brief Job mlutliple axis to requested target position
 *
 * Moves to a given position, but can move all 3 axes. Works simillary to jog_axis.
 *
 * @param position Input/output enqueued position of the axes
 * @param target Target position of the axis
 */
void jog_multiple_axis(xyz_float_t &position, const xyz_float_t target);

/**
 * @brief Helper to calculate correct moves when the user leaves the movement menu.
 *
 * We can't jug the axis anymore when user leaves the menu, so we need to plan remaining
 * steps as a whole. This helper calculates remaining steps for a multiaxis movement.
 *
 * @param position Last enqueued position
 * @param target Target position, where to move to
 */
void finish_movement(const xyz_float_t &position, const xyz_float_t &target);
