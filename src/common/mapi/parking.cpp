#include "parking.hpp"

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/libs/nozzle.h>

namespace mapi {
void park_move_with_conditional_home(const xyz_pos_t &park_position, ZAction z_action) {
    const xyz_bool_t do_axis { .x = !std::isnan(park_position.x), .y = !std::isnan(park_position.y), .z = !std::isnan(park_position.z) && z_action == mapi::ZAction::absolute_move };
    if (axes_need_homing(X_AXIS | Y_AXIS | Z_AXIS)) {
        GcodeSuite::G28_no_parser(true, 3, false, do_axis.x, do_axis.y, do_axis.z);
    }

    nozzle.park(std::to_underlying(z_action), park_position);
}
} // namespace mapi
