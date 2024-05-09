/**
 * @file menu_spin_config_with_units.cpp
 * @author Radek Vana
 * @brief included by menu_spin_config.hpp
 * menu_spin_config_basic.cpp must not be linked
 * @date 2020-11-04
 */
#include "menu_spin_config.hpp"
#include "config_features.h"

static constexpr const char *Celsius = "\xC2\xB0\x43"; // degree Celsius
static constexpr const char *Percent = "%";
static constexpr const char *None = "";
static constexpr const char *Hour = "h";
static constexpr const char *Minutes = "min";
static constexpr const char *mm = "mm";
static constexpr const char *mA = "mA";
static constexpr const char *rpm = "rpm"; // todo should I translate it?
static constexpr const char *Second = "s";
static constexpr const char *um = "um"; //"µm";

// SpinConfig_t == SpinConfigWithUnit
const SpinConfigInt SpinCnf::nozzle = SpinConfigInt(MenuVars::GetNozzleRange(), Celsius, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::bed = SpinConfigInt(MenuVars::GetBedRange(), Celsius, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::printfan = SpinConfigInt(MenuVars::percent_range, Percent, spin_off_opt_t::yes);
const SpinConfigInt SpinCnf::feedrate = SpinConfigInt(MenuVars::feedrate_range, Percent);
const SpinConfigInt SpinCnf::flowfact = SpinConfigInt(MenuVars::flowfact_range, Percent);
const SpinConfigInt SpinCnf::timezone = SpinConfigInt { { -12, 14, 1 }, Hour };
#if BOARD_IS_BUDDY
const SpinConfigInt SpinCnf::volume_range = { { 0, 11, 1 }, None, spin_off_opt_t::yes }; // crank it up to 11
#else
const SpinConfigInt SpinCnf::volume_range = { { 0, 3, 1 }, None, spin_off_opt_t::yes };
#endif
const SpinConfigInt SpinCnf::footer_center_N_range = { { 0, 5, 1 }, None, spin_off_opt_t::yes };
const SpinConfigInt SpinCnf::axis_z_max_range = SpinConfigInt(MenuVars::GetMaximumZRange(), mm);
const std::array<SpinConfigInt, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { SpinConfigInt(MenuVars::GetAxisRanges()[0], mm), SpinConfigInt(MenuVars::GetAxisRanges()[1], mm),
    SpinConfigInt(MenuVars::GetAxisRanges()[2], mm), SpinConfigInt(MenuVars::GetAxisRanges()[3], mm) } };
const SpinConfigInt SpinCnf::steps_per_unit = SpinConfigInt(MenuVars::steps_per_unit_range, None);
const SpinConfigInt SpinCnf::microstep_exponential = SpinConfigInt(MenuVars::microstep_exponential_range, None);
const SpinConfigInt SpinCnf::microstep_exponential_with_0 = SpinConfigInt(MenuVars::microstep_exponential_range_with_0, None);
const SpinConfigInt SpinCnf::rms_current = SpinConfigInt(MenuVars::axis_rms_currents_range, mA);
const SpinConfigInt SpinCnf::two_digits_uint = { { 0, 15, 1 }, None };
#if AXIS_DRIVER_TYPE_X(TMC2209)
const SpinConfigInt SpinCnf::crash_sensitivity = SpinConfigInt({ 0, 255, 1 }, None, spin_off_opt_t::no);
#elif AXIS_DRIVER_TYPE_X(TMC2130)
const SpinConfigInt SpinCnf::crash_sensitivity = SpinConfigInt({ -64, 63, 1 }, None, spin_off_opt_t::no);
#else
    #error "Unknown driver type."
#endif
const SpinConfigInt SpinCnf::crash_max_period = SpinConfigInt({ 0, 0xFFFFF, 1 }, None, spin_off_opt_t::no);

// private repo
#if PRINTER_IS_PRUSA_XL
const SpinConfigInt SpinCnf::fs_range = SpinConfigInt({ 50, 1500, 10 }, None);
#else
const SpinConfigInt SpinCnf::fs_range = SpinConfigInt({ 50000, 2500000, 1000 }, None);
#endif
const SpinConfigInt SpinCnf::loadcell_range = { { 5, 30, 1 }, None };
const SpinConfigInt SpinCnf::print_progress = SpinConfigInt({ 29, 200, 1 }, Second, spin_off_opt_t::yes); // lowest value is off
const SpinConfigInt SpinCnf::int_num = SpinConfigInt({ 0, std::numeric_limits<int32_t>::max(), 1 }, None, spin_off_opt_t::no);

#if PRINTER_IS_PRUSA_MK3_5
const SpinConfigInt SpinCnf::correction_range = SpinConfigInt({ -100, 100, 1 }, um, spin_off_opt_t::no);
#endif

const SpinConfigFlt SpinCnf::nozzle_diameter = SpinConfigFlt({ 0.25, 1.00, 0.05 }, mm, spin_off_opt_t::no, format_point2);
#if XL_ENCLOSURE_SUPPORT()
const SpinConfigInt SpinCnf::enclosure_fan = SpinConfigInt(MenuVars::enclosure_fan_percent_range, Percent);
const SpinConfigInt SpinCnf::enclosure_post_print = SpinConfigInt(MenuVars::enclosure_post_print_minute_range, Minutes);
#endif
