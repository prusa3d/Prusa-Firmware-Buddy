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
    static const SpinConfig_U16_t nozzle;
    static const SpinConfig_U08_t bed;
    static const SpinConfig_U08_t printfan;
    static const SpinConfig_U16_t feedrate;
    static const SpinConfig_U16_t flowfact;
    static const SpinConfig_I08_t timezone_range;
    static const SpinConfig_U08_t volume_range;
    static const SpinConfig_I08_t sensor_range;
    static const SpinConfig_U08_t footer_center_N_range;
    static const SpinConfig_I32_t axis_z_max_range;                            // maximum Z range - to change current range
    static const std::array<SpinConfig_I32_t, MenuVars::AXIS_CNT> axis_ranges; // current Z range
    static const SpinConfig_I32_t steps_per_unit;
    static const SpinConfig_I32_t microstep_exponential; // 2^0 - 2^8 .. 1, 2, 4, .. , 128, 256
    static const SpinConfig_I32_t rms_current;
};
