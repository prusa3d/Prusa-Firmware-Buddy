/**
 * @file algorithm_scale.hpp
 * @author Radek Vana
 * @brief template function set to scale numbers
 * originally used template <class OUT_TYPE, class IN_TYPE>
 * but I would need to know which type is bigger to use it to calculations
 * @date 2021-10-15
 */

#pragma once
#include <algorithm>

/**
 * @brief scale number from original to scaled range
 *
 * @tparam T
 * @param num number to be scaled
 * @param min original minimum range
 * @param max original maximum range
 * @param scaled_min scaled minimum range
 * @param scaled_max scaled maximum range
 * @return T constexpr
 */
template <class T>
T constexpr scale(T num, T min, T max, T scaled_min, T scaled_max) {
    if (scaled_min == scaled_max) {
        return scaled_min;
    }
    if (min == max) {
        return scaled_max;
    }

    if (min > max) {
        std::swap(min, max);
        std::swap(scaled_min, scaled_max);
    }

    num = std::clamp(num, min, max);

    if (scaled_min > scaled_max) {
        std::swap(scaled_min, scaled_max);
        num = num - min;
        num = max - num;
    }
    return (scaled_max - scaled_min) * (num - min) / (max - min) + scaled_min;
}

/**
 * @brief scale number from original range to percents
 *
 * @tparam T
 * @param num number to be scaled
 * @param min original minimum range
 * @param max original maximum range
 * @return T constexpr
 */
template <class T>
T constexpr scale_percent(T num, T min, T max) {
    return scale(num, min, max, T(0), T(100));
}

/**
 * @brief scale number from original to scaled range
 * moves scaled number and its original range to virtual zero to avoid overflow
 * usefull to convert timestamps to something else
 * @tparam T
 * @param num number to be scaled
 * @param min original minimum range
 * @param max original maximum range
 * @param scaled_min scaled minimum range
 * @param scaled_max scaled maximum range
 * @return T constexpr
 */
template <class T>
T constexpr scale_avoid_overflow(T num, T min, T max, T scaled_min, T scaled_max) {
    num -= min;
    max -= min;
    min -= min; // better than min = 0, type safe
    return scale(num, min, max, scaled_min, scaled_max);
}

/**
 * @brief scale number from original range to percents
 * moves scaled number and its original range to virtual zero to avoid overflow
 * usefull to convert timestamps to progress
 * @tparam T
 * @param num number to be scaled
 * @param min original minimum range
 * @param max original maximum range
 * @return T constexpr
 */
template <class T>
T constexpr scale_percent_avoid_overflow(T num, T min, T max) {
    return scale_avoid_overflow(num, min, max, T(0), T(100));
}
