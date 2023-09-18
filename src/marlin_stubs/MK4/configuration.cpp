#include "configuration.hpp"

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

uint32_t get_stall_threshold() {
    return config_store().xy_motors_400_step.get() ? 80 : 400;
}
