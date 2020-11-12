/**
 * @file menu_spin_config_type.hpp
 * @author Radek Vana
 * @brief select SpinConfig type by printer configuration
 * @date 2020-11-04
 */
#pragma once

#include "menu_spin_config_types.hpp"
#include "printers.h"
#include <cstdint>

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
template <class T>
using SpinConfig_t = SpinConfig<T>;
#else
template <class T>
using SpinConfig_t = SpinConfigWithUnit<T>;
#endif

using SpinConfig_U08_t = SpinConfig_t<uint8_t>;
using SpinConfig_I08_t = SpinConfig_t<int8_t>;

using SpinConfig_U16_t = SpinConfig_t<uint16_t>;
using SpinConfig_I16_t = SpinConfig_t<int16_t>;

using SpinConfig_U32_t = SpinConfig_t<uint32_t>;
using SpinConfig_I32_t = SpinConfig_t<int32_t>;
