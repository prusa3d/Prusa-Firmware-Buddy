/**
 * @file menu_spin_config_basic.cpp
 * @author Radek Vana
 * @brief included by menu_spin_config.hpp
 * menu_spin_config_with_units.cpp must not be linked
 * @date 2020-11-04
 */
#include "menu_spin_config.hpp"
#include "config_features.h"

// SpinConfig_t == SpinConfig
const SpinConfigInt SpinCnf::nozzle = SpinConfigInt(MenuVars::GetNozzleRange());
const SpinConfigInt SpinCnf::bed = SpinConfigInt(MenuVars::GetBedRange());
const SpinConfigInt SpinCnf::printfan = SpinConfigInt(MenuVars::percent_range, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::feedrate = SpinConfigInt(MenuVars::feedrate_range);
const SpinConfigInt SpinCnf::flowfact = SpinConfigInt(MenuVars::flowfact_range);
const SpinConfigInt SpinCnf::timezone = { { -12, 14, 1 } };
const SpinConfigInt SpinCnf::volume_range = { { 0, 11, 1 } }; // crank it up to 11
const SpinConfigInt SpinCnf::footer_center_N_range = { { 0, 3, 1 } };
const SpinConfigInt SpinCnf::axis_z_max_range = SpinConfigInt(MenuVars::GetMaximumZRange());
const std::array<SpinConfigInt, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { SpinConfigInt(MenuVars::GetAxisRanges()[0]), SpinConfigInt(MenuVars::GetAxisRanges()[1]),
    SpinConfigInt(MenuVars::GetAxisRanges()[2]), SpinConfigInt(MenuVars::GetAxisRanges()[3]) } };
const SpinConfigInt SpinCnf::steps_per_unit = SpinConfigInt(MenuVars::steps_per_unit_range);
const SpinConfigInt SpinCnf::microstep_exponential = SpinConfigInt(MenuVars::microstep_exponential_range);
const SpinConfigInt SpinCnf::microstep_exponential_with_0 = SpinConfigInt(MenuVars::microstep_exponential_range_with_0);
const SpinConfigInt SpinCnf::rms_current = SpinConfigInt(MenuVars::axis_rms_currents_range);
const SpinConfigInt SpinCnf::two_digits_uint = { { 0, 15, 1 } };
#if AXIS_DRIVER_TYPE_X(TMC2209)
const SpinConfigInt SpinCnf::crash_sensitivity = SpinConfigInt({ 0, 255, 1 }, spin_off_opt_t::no);
#elif AXIS_DRIVER_TYPE_X(TMC2130)
const SpinConfigInt SpinCnf::crash_sensitivity = SpinConfigInt({ -64, 63, 1 }, spin_off_opt_t::no);
#else
    #error "Unknown driver type."
#endif
const SpinConfigInt SpinCnf::crash_max_period = SpinConfigInt({ 0, 0xFFFFF, 1 }, spin_off_opt_t::no);

// private repo
const SpinConfigInt SpinCnf::fs_range = SpinConfigInt({ 50000, 2500000, 1000 });
const SpinConfigInt SpinCnf::loadcell_range = SpinConfigInt({ 5, 30, 1 });
const SpinConfigInt SpinCnf::print_progress = SpinConfigInt({ 30, 200, 1 });
const SpinConfigInt SpinCnf::int_num = SpinConfigInt({ 0, std::numeric_limits<int32_t>::max(), 1 }, spin_off_opt_t::no);

#if PRINTER_IS_PRUSA_MINI
const SpinConfigInt SpinCnf::correction_range = SpinConfigInt({ -100, 100, 1 }, spin_off_opt_t::no);
#endif

const SpinConfigFlt SpinCnf::nozzle_diameter = SpinConfigFlt({ 0.25, 1.00, 0.05 }, spin_off_opt_t::no, format_point2);
