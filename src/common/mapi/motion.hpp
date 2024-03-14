#pragma once

namespace mapi {

/**
 * Moves the extruder. Properly respects MBL.
 *
 * @param distance The distance to move in mm.
 * @param feed_rate The feed rate of the move in mm/s.
 */
void extruder_move(float distance, float feed_rate);

/**
 * Schedules a short extruder move, unless there are enough (3) moves already
 * in the queue.
 *
 * Use this function to emulate continuous extruder turning by calling it
 * periodically. You can adjust `step` to a longer or shorter granularity
 * depending on how often you call this function. This also determines how long
 * will the extruder keep turning after you stop calling this function.
 *
 * @param feed_rate The feed rate of the move in mm/s.
 * @param step The step in mm to schedule (not related to motor steps).
 */
float extruder_schedule_turning(float feed_rate, float step = 0.6);

} // namespace mapi
