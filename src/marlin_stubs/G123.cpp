#include "PrusaGcodeSuite.hpp"

#include <Marlin.h>
#include <gcode/gcode_parser.hpp>
#include <module/motion.h>
#include <module/planner.h>
#include <gcode/queue.h>
#include <scope_guard.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G123: Manual move
 *
 * Moves axes to the target coordinates at MANUAL_FEEDRATE.
 * Stops the movement if a new gcode gets queued.
 *
 *#### Parameters
 *
 *  - `X` - Target X position
 *  - `Y` - Target Y position
 *  - `Z` - Target Z position
 *  - `E` - Target E position
 */

void PrusaGcodeSuite::G123() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    xyze_float_t target_pos = current_position;
    p.store_option('X', target_pos.x);
    p.store_option('Y', target_pos.y);
    p.store_option('Z', target_pos.z);
    p.store_option('E', target_pos.e);

    static const xyze_float_t feedrate = xyze_float_t { { MANUAL_FEEDRATE } } / 60;
    static const xyze_float_t feedrate_inv = xyze_float_t { { { 1, 1, 1, 1 } } } / feedrate;

    // The segments should be bit enough so that the planner does not have to emergency break,
    // but also short enough so that we have snappy reaction time
    static constexpr float max_segment_duration_s = 0.02f;

    const auto prev_settings = planner.user_settings;
    ScopeGuard _sg = [&] { planner.apply_settings(prev_settings); };

    // Reset to default parameters for the duration of this gcode
    Motion_Parameters::reset();

    while (true) {
        // Check this at the end - we want to plan at least one segment
        // The first gcode in the queue should be the currently running G123 (if the gcode was not injected, then everything is a bit different, hehe)
        if (queue.length > 1) {
            return;
        }

        // Only keep a minimum amount of blocks planned so that we can stop the motion fast
        if (planner.is_full()) {
            idle(true);
            continue;
        }

        const auto distance_to_target = target_pos - current_position;
        float segment_duration = max_segment_duration_s;
        float segment_feedrate = 10000;

        static const xyze_float_t empty_segment_dir { { { 0, 0, 0, 0 } } };
        xyze_float_t segment_dir = empty_segment_dir;

        for (uint8_t i = 0; i < 4; i++) {
            if (std::abs(distance_to_target[i]) < 0.01f) {
                continue;
            }

            // We're actually not able to drive the axes independently, so we just go with the slowest feedrate
            segment_duration = std::min(segment_duration, std::abs(distance_to_target[i]) * feedrate_inv[i]);
            segment_feedrate = std::min(segment_feedrate, feedrate[i]);
            segment_dir[i] = std::copysign(1, distance_to_target[i]);
        }

        if (segment_dir == empty_segment_dir) {
            // We've reached the destination and are done
            return;
        }

        const auto dir_feedrate = segment_dir * feedrate;
        current_position += dir_feedrate * segment_duration;

        // Use dir_feedrate - excludes axes that we're not moving
        line_to_current_position(std::max(segment_feedrate, dir_feedrate.magnitude()));
    }
}

/** @}*/
