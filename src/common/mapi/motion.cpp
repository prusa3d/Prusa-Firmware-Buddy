#include "motion.hpp"

#include <Marlin/src/module/planner.h>

#include "src/module/motion.h"

namespace mapi {

void extruder_move(float distance, float feed_rate) {
    destination = current_position;
    destination[3] += distance;
    feedrate_mm_s = feed_rate;

    prepare_move_to_destination();
}

float extruder_schedule_turning(float feed_rate, float step) {
    if (planner.movesplanned() <= 3) {
        extruder_move(step, feed_rate);
        return step;
    }

    return 0;
}

} // namespace mapi
