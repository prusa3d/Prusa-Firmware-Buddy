#include "marlin_move.hpp"

#include "marlin_vars.hpp"
#include "src/module/motion.h"

namespace marlin {

void extruder_move(float distance, float feed_rate) {
    destination = current_position;
    destination[3] += distance;
    feedrate_mm_s = feed_rate;

    prepare_move_to_destination();
}

float extruder_schedule_turning(float feed_rate, float step) {
    if (marlin_vars()->pqueue <= 3) {
        extruder_move(step, feed_rate);
        return step;
    }

    return 0;
}

} // namespace marlin
