/**
 * @file menu_spin_config_basic.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 *
 * cannot be included together with menu_spin_config_with_units
 * do not include this file directly
 * include menu_spin_config instead
 */
#pragma once

#include "menu_spin_config_type.hpp"
#include "menu_vars.h"
#include "window_types.hpp" //SENSOR_STATE
namespace DoNotUse {        //do not use this except for cpp file
struct SpinCnf_basic {
    static constexpr SpinConfig_I08_t timezone_range = { { -12, 12, 1 } };
    static constexpr SpinConfig_U08_t volume_range = { { 0, 10, 1 } };
    static constexpr SpinConfig_I08_t sensor_range = { { (int8_t)SENSOR_STATE::unknown, (int8_t)SENSOR_STATE::high, 1 } }; /// min value, max value, step
    //todo make constexpr
    static const SpinConfig_U16_t nozzle;
    static const SpinConfig_U08_t bed;
    static const SpinConfig_U08_t printfan;
    static const SpinConfig_U16_t feedrate;
    static const SpinConfig_U16_t flowfact;
    static const std::array<SpinConfig_I16_t, MenuVars::AXIS_CNT> axis_ranges;
};
};
