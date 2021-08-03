/**
 * @file menu_spin_config_basic.cpp
 * @author Radek Vana
 * @brief included by menu_spin_config.hpp
 * menu_spin_config_with_units.cpp must not be linked
 * @date 2020-11-04
 */
#include "menu_spin_config.hpp"
#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfigPre.h"

// SpinConfig_t == SpinConfig
const SpinConfig_U16_t SpinCnf::nozzle = SpinConfig_U16_t(MenuVars::nozzle_range);
const SpinConfig_U08_t SpinCnf::bed = SpinConfig_U08_t(MenuVars::bed_range);
const SpinConfig_U08_t SpinCnf::printfan = SpinConfig_U08_t(MenuVars::printfan_range);
const SpinConfig_U16_t SpinCnf::feedrate = SpinConfig_U16_t(MenuVars::feedrate_range);
const SpinConfig_U16_t SpinCnf::flowfact = SpinConfig_U16_t(MenuVars::flowfact_range);
const SpinConfig_I08_t SpinCnf::timezone_range = { { -12, 12, 1 } };
const SpinConfig_U08_t SpinCnf::volume_range = { { 0, 11, 1 } }; //crank it up to 11
const std::array<SpinConfig_I16_t, MenuVars::AXIS_CNT> SpinCnf::axis_ranges = { { SpinConfig_I16_t(MenuVars::axis_ranges[0]), SpinConfig_I16_t(MenuVars::axis_ranges[1]),
    SpinConfig_I16_t(MenuVars::axis_ranges[2]), SpinConfig_I16_t(MenuVars::axis_ranges[3]) } };
const SpinConfig_I32_t SpinCnf::axis_z_range = { { Z_MIN_LEN_LIMIT, Z_MAX_LEN_LIMIT, 1 } };
