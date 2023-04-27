/**
 * @file menu_spin_config_type.hpp
 * @author Radek Vana
 * @brief select SpinConfig type by printer configuration
 * @date 2020-11-04
 */
#pragma once

#include "menu_spin_config_types.hpp"
#include "printers.h"

#if ((PRINTER_TYPE == PRINTER_PRUSA_MK4) || (PRINTER_TYPE == PRINTER_PRUSA_IXL) || (PRINTER_TYPE == PRINTER_PRUSA_XL))
template <class T>
using SpinConfig_t = SpinConfigWithUnit<T>;
#else
template <class T>
using SpinConfig_t = SpinConfig<T>;
#endif

using SpinConfigInt = SpinConfig_t<int>;
