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
static constexpr const char *rpm = "rpm"; //todo should I translate it?

// SpinConfig_t == SpinConfigWithUnit
const SpinConfig_U16_t SpinCnf::nozzle = SpinConfig_U16_t(MenuVars::nozzle_range, Celsius);
const SpinConfig_U08_t SpinCnf::bed = SpinConfig_U08_t(MenuVars::bed_range, Celsius);
const SpinConfig_U08_t SpinCnf::printfan = SpinConfig_U08_t(MenuVars::printfan_range, rpm);
const SpinConfig_U16_t SpinCnf::feedrate = SpinConfig_U16_t(MenuVars::feedrate_range, Percent);
const SpinConfig_U16_t SpinCnf::flowfact = SpinConfig_U16_t(MenuVars::flowfact_range, Percent);
const SpinConfig_I08_t SpinCnf::timezone_range = { { -12, 12, 1 }, Hour };
const SpinConfig_U08_t SpinCnf::volume_range = { { 0, 11, 1 }, None }; //crank it up to 11
const std::array<SpinConfig_I16_t, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { axis_ranges[0], mm }, { axis_ranges[1], mm }, { axis_ranges[2], mm }, { axis_ranges[3], mm } };
