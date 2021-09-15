/**
 * @file menu_spin_config_types.cpp
 * @author Radek Vana
 * @date 2020-11-05
 */
#include "menu_spin_config_types.hpp"

template <>
const char *const SpinConfig<int>::prt_format = "%d";

template <>
const char *const SpinConfig<float>::prt_format = "%f";
