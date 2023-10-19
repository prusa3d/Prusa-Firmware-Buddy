/**
 * @file menu_spin_config.hpp
 * @author Radek Vana
 * @brief included by menu_spin_config_basic.cpp xor menu_spin_config_with_units.cpp
 * @date 2020-11-04
 */
#pragma once
#include <array>
#include "menu_spin_config_type.hpp"
#include "menu_vars.h"

struct SpinCnf {
    static const SpinConfigInt nozzle;
    static const SpinConfigInt bed;
    static const SpinConfigInt printfan;
    static const SpinConfigInt feedrate;
    static const SpinConfigInt flowfact;
    static const SpinConfigInt timezone_range;
    static const SpinConfigInt volume_range;
    static const SpinConfigInt sensor_range;
    static const SpinConfigInt footer_center_N_range;
    static const SpinConfigInt axis_z_max_range; // maximum Z range - to change current range
    static const std::array<SpinConfigInt, MenuVars::AXIS_CNT> axis_ranges; // current Z range
    static const SpinConfigInt steps_per_unit;
    static const SpinConfigInt microstep_exponential; // 2^0 - 2^8 .. 1, 2, 4, .. , 128, 256
    static const SpinConfigInt microstep_exponential_with_0;
    static const SpinConfigInt rms_current;
    static const SpinConfigInt two_digits_uint;
    static const SpinConfigInt crash_sensitivity;
    static const SpinConfigInt crash_max_period;

    // private repo
    static const SpinConfigInt fs_range;
    static const SpinConfigInt loadcell_range;
    static const SpinConfigInt print_progress;
    static const SpinConfigInt int_num;

    static const SpinConfigFlt nozzle_diameter;
};
