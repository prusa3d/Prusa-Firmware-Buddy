/**
 * @file menu_spin_config_type.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 *
 */
#pragma once

#include "menu_spin_config_types.hpp"
#include <stdint.h>

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
template <class T>
using SpinConfig_t = SpinConfig<T>;
#else
template <class T>
using SpinConfig_t = SpinConfigWithUnit<T>;
#endif

using SpinConfig_U08_t = SpinConfig<uint8_t>;
using SpinConfig_I08_t = SpinConfig<int8_t>;

using SpinConfig_U16_t = SpinConfig<uint16_t>;
using SpinConfig_I16_t = SpinConfig<int16_t>;

using SpinConfig_U32_t = SpinConfig<uint32_t>;
using SpinConfig_I32_t = SpinConfig<int32_t>;
