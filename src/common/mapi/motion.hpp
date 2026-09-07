#pragma once

namespace mapi {

/**
 * Moves the extruder. Properly respects MBL.
 *
 * @param distance The distance to move in mm.
 * @param feed_rate The feed rate of the move in mm/s.
 * @param ignore_flow_factor Ignore user-set flow factor (set planner.e_factor = 1 for the move)
 * @returns if the move was successfully queued
 */
bool extruder_move(float distance, float feed_rate, bool ignore_flow_factor = true);

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
float extruder_schedule_turning(float feed_rate, float step = 0.6f);

/// Deretracts any retraction of the filament known to the printer
/// Considers auto_retract and filament_tracker
/// Blocks until the deretraction is done
void fully_deretract(float fr_mm_s);

/// Ensures the filament is retracted to the specified distance
/// Considers auto_retract and filament_tracker
/// Blocks
void retract_to(float target_retraction_distance, float fr_mm_s);

/// Moves the extruder so the filament ends up retracted exactly
/// Unlike retract_to, may move in either direction.
/// Considers auto_retract and filament_tracker
/// Blocks
void restore_retracted_distance(float target_retraction_distance, float fr_mm_s);

/**
 * Make sure a tool with accelerometer is picked up. If not, pick up such a
 * tool. Does nothing on machines without a toolchanger or accelerometer.
 */
void ensure_tool_with_accelerometer_picked();

} // namespace mapi
