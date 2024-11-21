#include "configuration.hpp"

#include "feature/tmc_util.h"
#include <config_store/store_instance.hpp>

float axis_home_min_diff(uint8_t axis_num) {
    if (axis_num == Z_AXIS) {
        return axis_home_min_diff_z;
    }

    // Revisit this switch if another printer is added
    static_assert(extended_printer_type_model.size() == 4);
    switch (PrinterModelInfo::current().model) {

    case PrinterModel::mk3_9:
    case PrinterModel::mk3_9s:
        return axis_home_min_diff_xy_mk3_9;

    default:
        return axis_home_min_diff_xy_mk4;
    }
}

float axis_home_max_diff(uint8_t axis_num) {
    if (axis_num == Z_AXIS) {
        return axis_home_max_diff_z;
    }

    // Revisit this switch if another printer is added
    static_assert(extended_printer_type_model.size() == 4);
    switch (PrinterModelInfo::current().model) {

    case PrinterModel::mk3_9:
    case PrinterModel::mk3_9s:
        return axis_home_max_diff_xy_mk3_9;

    default:
        return axis_home_max_diff_xy_mk4;
    }
}
