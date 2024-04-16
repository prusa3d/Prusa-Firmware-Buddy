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

    // We cannot work with current_position, because current_position might or might not have MBL applied on the Z axis.
    // So we gotta use planner.position_float, which should always be matching.
    auto pos = planner.position_float;
    pos.e += distance;

    // But we gotta update current_position.e, too. .e should be always the same with planner.position_float (hopefully).
    // Only .z should ever differ because of MBL application.
    current_position.e = pos.e;

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
