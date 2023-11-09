#include "motion.hpp"

#include <Marlin/src/module/planner.h>

#include "RAII.hpp"
#include "src/module/motion.h"

namespace mapi {

bool extruder_move(float distance, float feed_rate, bool ignore_flow_factor) {
    // Dry run - only simulate extruder moves
    if (DEBUGGING(DRYRUN)) {
        return true;
    }

    // Temporarily reset extrusion factor, if ignore_flow_factor
    AutoRestore _ef(planner.e_factor[active_extruder], 1.0f, ignore_flow_factor);

    current_position.e += distance;
    auto pos = current_position;

    // Gotta apply leveling, otherwise the move would move the axes to non-leveled coordinates
    // If PLANNER_LEVELING is true, the leveling is applied inside buffer_line
#if HAS_LEVELING && !PLANNER_LEVELING
    planner.apply_leveling(pos);
#endif

    return planner.buffer_line(pos, feed_rate);
}

float extruder_schedule_turning(float feed_rate, float step) {
    if (planner.movesplanned() <= 3) {
        extruder_move(step, feed_rate);
        return step;
    }

    return 0;
}

} // namespace mapi
