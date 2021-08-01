/**
 * @file menu_spin_config_with_units.cpp
 * @author Radek Vana
 * @brief included by menu_spin_config.hpp
 * menu_spin_config_basic.cpp must not be linked
 * @date 2020-11-04
 */
#include "menu_spin_config.hpp"

static constexpr const char *Celsius = "\177C";
static constexpr const char *Percent = "%";
static constexpr const char *None = "";
static constexpr const char *Hour = "h";
static constexpr const char *mm = "mm";
static constexpr const char *mA = "mA";
static constexpr const char *rpm = "rpm"; //todo should I translate it?

// SpinConfig_t == SpinConfigWithUnit
const SpinConfigInt SpinCnf::nozzle = SpinConfigInt(MenuVars::nozzle_range, Celsius, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::bed = SpinConfigInt(MenuVars::bed_range, Celsius, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::printfan = SpinConfigInt(MenuVars::printfan_range, rpm, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::feedrate = SpinConfigInt(MenuVars::feedrate_range, Percent);
const SpinConfigInt SpinCnf::flowfact = SpinConfigInt(MenuVars::flowfact_range, Percent);
const SpinConfigInt SpinCnf::timezone_range = { { -12, 12, 1 }, Hour };
const SpinConfigInt SpinCnf::volume_range = { { 0, 11, 1 }, None, spin_off_opt_t::yes }; //crank it up to 11
const SpinConfigInt SpinCnf::footer_center_N_range = { { 0, 5, 1 }, None, spin_off_opt_t::yes };
const SpinConfigInt SpinCnf::axis_z_max_range = SpinConfigInt(MenuVars::maximum_z_axis_range), mm;
const std::array<SpinConfigInt, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { SpinConfigInt(MenuVars::axis_ranges[0], mm), SpinConfigInt(MenuVars::axis_ranges[1], mm),
    SpinConfigInt(MenuVars::axis_ranges[2], mm), SpinConfigInt(MenuVars::axis_ranges[3], mm) } };
const SpinConfigInt SpinCnf::steps_per_unit = SpinConfigInt(MenuVars::steps_per_unit_range, None);
const SpinConfigInt SpinCnf::microstep_exponential = SpinConfigInt(MenuVars::microstep_exponential_range, None);
const SpinConfigInt SpinCnf::rms_current = SpinConfigInt(MenuVars::axis_rms_currents_range, mA);
