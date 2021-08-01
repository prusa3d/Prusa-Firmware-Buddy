/**
 * @file menu_spin_config_type.hpp
 * @author Radek Vana
 * @brief select SpinConfig type by printer configuration
 * @date 2020-11-04
 */
#pragma once

#include "menu_spin_config_types.hpp"
#include "printers.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
template <class T>
using SpinConfig_t = SpinConfig<T>;
#else
template <class T>
using SpinConfig_t = SpinConfigWithUnit<T>;
#endif

using SpinConfigInt = SpinConfig_t<int>;
