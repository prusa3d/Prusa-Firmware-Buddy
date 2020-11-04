/**
 * @file menu_spin_config_basic.cpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 *
 */
#include "menu_spin_config_basic.hpp"
const SpinConfig_I08_t DoNotUse::SpinCnf_basic::timezone_range;
const SpinConfig_U08_t DoNotUse::SpinCnf_basic::volume_range;
const SpinConfig_I08_t DoNotUse::SpinCnf_basic::sensor_range;

const SpinConfig_U16_t DoNotUse::SpinCnf_basic::nozzle = SpinConfig_U16_t(MenuVars::nozzle_range);
const SpinConfig_U08_t DoNotUse::SpinCnf_basic::bed = SpinConfig_U08_t(MenuVars::bed_range);
const SpinConfig_U08_t DoNotUse::SpinCnf_basic::printfan = SpinConfig_U08_t(MenuVars::printfan_range);
const SpinConfig_U16_t DoNotUse::SpinCnf_basic::feedrate = SpinConfig_U16_t(MenuVars::feedrate_range);
const SpinConfig_U16_t DoNotUse::SpinCnf_basic::flowfact = SpinConfig_U16_t(MenuVars::flowfact_range);
const std::array<SpinConfig_I16_t, MenuVars::AXIS_CNT> DoNotUse::SpinCnf_basic::axis_ranges = { axis_ranges };
