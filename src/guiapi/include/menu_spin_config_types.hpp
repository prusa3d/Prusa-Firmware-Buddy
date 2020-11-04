/**
 * @file menu_spin_config_types.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 * do not include this file outside menu_spin_config_type.hpp
 * include menu_spin_config_type.hpp instead
 */
#pragma once
#include <array>

template <class T>
struct SpinConfig {
    std::array<T, 3> range; // todo change array to struct containing min, max, step
    constexpr SpinConfig(const std::array<T, 3> &arr)
        : range(arr) {}
};

template <class T>
struct SpinConfigWithUnit : public SpinConfig<T> {
    const char *unit;
    constexpr SpinConfigWithUnit(const std::array<T, 3> &arr, const char *unit_)
        : SpinConfig<T>(arr)
        , unit(unit_) {}
};
