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

// const char* Unit() is not virtual, because only one of SpinConfig SpinConfigWithUnit is used

template <class T>
struct SpinConfig {
    static constexpr const char *nullstr = "";
    std::array<T, 3> range; // todo change array to struct containing min, max, step
    static const char *const prt_format;

    constexpr SpinConfig(const std::array<T, 3> &arr)
        : range(arr) {}
    constexpr T Min() { return range[0]; }
    constexpr T Max() { return range[1]; }
    constexpr T Step() { return range[2]; }
    constexpr const char *Unit() { return nullstr; } // not virtual
};

template <class T>
struct SpinConfigWithUnit : public SpinConfig<T> {
    const char *const unit;

    constexpr SpinConfigWithUnit(const std::array<T, 3> &arr, const char *unit_)
        : SpinConfig<T>(arr)
        , unit(unit_) {}
    constexpr const char *Unit() { return unit; } // not virtual
};
