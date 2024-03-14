/**
 * @file menu_spin_config_types.hpp
 * @author Radek Vana
 * @brief do not include this file outside menu_spin_config_type.hpp
 * include menu_spin_config_type.hpp instead
 * @date 2020-11-04
 */
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <stdio.h>
#include "i18n.h"
// const char* Unit() is not virtual, because only one of SpinConfig SpinConfigWithUnit is used

enum class spin_off_opt_t : bool { no,
    yes }; // yes == lowest value is off

union SpinType {
    float flt;
    int i;

    constexpr operator float() const { return flt; }
    constexpr operator int() const { return i; }

    constexpr SpinType(float x)
        : flt(x) {}
    constexpr SpinType(int x)
        : i(x) {}
};

template <class T>
struct SpinConfig {
    std::array<T, 3> range; // todo change array to struct containing min, max, step
    static const char *const prt_format;
    const char *const prt_format_override;
    spin_off_opt_t off_opt;
    static constexpr const char *const off_opt_str = N_("Off");

    constexpr SpinConfig(const std::array<T, 3> &arr, spin_off_opt_t off_opt_ = spin_off_opt_t::no, const char *const format_override = nullptr)
        : range(arr)
        , prt_format_override(format_override)
        , off_opt(off_opt_) {}
    constexpr T Min() const { return range[0]; }
    constexpr T Max() const { return range[1]; }
    constexpr T Step() const { return range[2]; }
    constexpr const char *Unit() const { return nullptr; } // not virtual
    bool IsOffOptionEnabled() const { return off_opt == spin_off_opt_t::yes; }

    size_t txtMeas(T val) const;

    // calculate all possible values
    size_t calculateMaxDigits() const {
        size_t max_len = txtMeas(Max());
        for (T step_sum = Min(); step_sum < Max(); step_sum += Step()) {
            size_t len = txtMeas(step_sum);
            max_len = std::max(len, max_len);
        }
        return max_len;
    }
};

template <class T>
size_t SpinConfig<T>::txtMeas(T val) const {
    if (IsOffOptionEnabled() && val == Min()) {
        return _(off_opt_str).computeNumUtf8CharsAndRewind();
    } else {
        return snprintf(nullptr, 0, prt_format_override ? prt_format_override : prt_format, val);
    }
}

template <>
inline size_t SpinConfig<float>::txtMeas(float val) const {
    if (IsOffOptionEnabled() && val == Min()) {
        return _(off_opt_str).computeNumUtf8CharsAndRewind();
    } else {
        return snprintf(nullptr, 0, prt_format_override ? prt_format_override : prt_format, (double)val);
    }
}

template <class T>
struct SpinConfigWithUnit : public SpinConfig<T> {
    const char *const unit;

    constexpr SpinConfigWithUnit(const std::array<T, 3> &arr, const char *unit_, spin_off_opt_t off_opt_ = spin_off_opt_t::no, const char *const format = nullptr)
        : SpinConfig<T>(arr, off_opt_, format)
        , unit(unit_) {}
    constexpr const char *Unit() const { return unit; } // not virtual
};
