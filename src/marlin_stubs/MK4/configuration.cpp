#include "configuration.hpp"

#include "feature/tmc_util.h"

float axis_home_min_diff(uint8_t axis_num) {
    switch (axis_num) {
    case 0:
    case 1:
        return config_store().xy_motors_400_step.get() ? axis_home_min_diff_xy_mk4 : axis_home_min_diff_xy_mk3_9;
    case 2:
        return axis_home_min_diff_z;
    }
    return NAN;
}

float axis_home_max_diff(uint8_t axis_num) {
    switch (axis_num) {
    case 0:
    case 1:
        return config_store().xy_motors_400_step.get() ? axis_home_max_diff_xy_mk4 : axis_home_max_diff_xy_mk3_9;
    case 2:
        return axis_home_max_diff_z;
    }
    return NAN;
}

uint32_t get_homing_stall_threshold(AxisEnum axis_id) {
    switch (axis_id) {
    case X_AXIS:
    case Y_AXIS:
        return tmc_period_to_feedrate(X_AXIS, get_microsteps_x(), HOMING_FEEDRATE_XY / 60 * 0.8, get_steps_per_unit_x());
    case Z_AXIS:
        return 80; // this value may or may not be correct
    default:
        bsod("Wrong axis for homing stall threshold");
    }
}
