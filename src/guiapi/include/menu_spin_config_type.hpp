/**
 * @file menu_spin_config_type.hpp
 * @author Radek Vana
 * @brief select SpinConfig type by printer configuration
 * @date 2020-11-04
 */
#pragma once

#include "menu_spin_config_types.hpp"
#include "printers.h"

#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL)
template <class T>
using SpinConfig_t = SpinConfigWithUnit<T>;

template <class T>
constexpr inline SpinConfig_t<T> makeSpinConfig(const std::array<T, 3> &arr, const char *unit, spin_off_opt_t off_opt) {
    return { arr, unit, off_opt };
}
#else
template <class T>
using SpinConfig_t = SpinConfig<T>;

template <class T>
constexpr inline SpinConfig_t<T> makeSpinConfig(const std::array<T, 3> &arr, const char *, spin_off_opt_t off_opt) {
    return { arr, off_opt };
}
#endif

using SpinConfigInt = SpinConfig_t<int>;
using SpinConfigFlt = SpinConfig_t<float>;

constexpr const char *const format_point2 = "%0.2f";
