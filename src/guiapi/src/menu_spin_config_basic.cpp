/**
 * @file menu_spin_config_basic.cpp
 * @author Radek Vana
 * @brief included by menu_spin_config.hpp
 * menu_spin_config_with_units.cpp must not be linked
 * @date 2020-11-04
 */
#include "menu_spin_config.hpp"

// SpinConfig_t == SpinConfig
const SpinConfigInt SpinCnf::nozzle = SpinConfigInt(MenuVars::nozzle_range);
const SpinConfigInt SpinCnf::bed = SpinConfigInt(MenuVars::bed_range);
const SpinConfigInt SpinCnf::printfan = SpinConfigInt(MenuVars::printfan_range);
const SpinConfigInt SpinCnf::feedrate = SpinConfigInt(MenuVars::feedrate_range);
const SpinConfigInt SpinCnf::flowfact = SpinConfigInt(MenuVars::flowfact_range);
const SpinConfigInt SpinCnf::timezone_range = { { -12, 12, 1 } };
const SpinConfigInt SpinCnf::volume_range = { { 0, 11, 1 } }; //crank it up to 11
const SpinConfigInt SpinCnf::footer_center_N_range = { { 0, 3, 1 } };
const SpinConfigInt SpinCnf::axis_z_max_range = SpinConfigInt(MenuVars::maximum_z_axis_range);
const std::array<SpinConfigInt, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { SpinConfigInt(MenuVars::axis_ranges[0]), SpinConfigInt(MenuVars::axis_ranges[1]),
    SpinConfigInt(MenuVars::axis_ranges[2]), SpinConfigInt(MenuVars::axis_ranges[3]) } };
const SpinConfigInt SpinCnf::steps_per_unit = SpinConfigInt(MenuVars::steps_per_unit_range);
const SpinConfigInt SpinCnf::microstep_exponential = SpinConfigInt(MenuVars::microstep_exponential_range);
const SpinConfigInt SpinCnf::rms_current = SpinConfigInt(MenuVars::axis_rms_currents_range);
