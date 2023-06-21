/**
 * @file algorithm_range.hpp
 * @author Radek Vana
 * @brief set of function to evaluate if an element is in given interval
 * @date 2021-09-30
 */

#pragma once

#include <concepts>

template <typename T>
constexpr bool IsInOpenRange(const T &value, const T &left, const T &right);
template <typename T>
constexpr bool IsInClosedRange(const T &value, const T &left, const T &right);
template <typename T>
constexpr bool IsInLeftOpenRange(const T &value, const T &left, const T &right);
template <typename T>
constexpr bool IsInRightOpenRange(const T &value, const T &left, const T &right);

// only operator < used .. fewer dependencies
// everithing is private, only friend functions can use it
template <class T>
class range__ {
    static constexpr bool open_interval(const T &value, const T &low, const T &high) {
        return (low < value) && (value < high);
    }
    static constexpr bool closed_interval(const T &value, const T &low, const T &high) {
        if constexpr (std::floating_point<T>) {
            return value >= low && value <= high;
        } else {
            return !(value < low) && !(high < value);
        }
    }
    static constexpr bool left_open_interval(const T &value, const T &low, const T &high) {
        return (low < value) && !(high < value);
    }
    static constexpr bool right_open_interval(const T &value, const T &low, const T &high) {
        return !(value < low) && (value < high);
    }
    friend bool IsInOpenRange<>(const T &, const T &, const T &);
    friend bool IsInClosedRange<>(const T &, const T &, const T &);
    friend bool IsInLeftOpenRange<>(const T &, const T &, const T &);
    friend bool IsInRightOpenRange<>(const T &, const T &, const T &);

    range__() = delete;
    range__(const range__ &) = delete;
};

template <typename T>
constexpr bool IsInOpenRange(const T &value, const T &left, const T &right) {
    return (left < right) ? range__<T>::open_interval(value, left, right) : range__<T>::open_interval(value, right, left);
}

template <typename T>
constexpr bool IsInClosedRange(const T &value, const T &left, const T &right) {
    return (left < right) ? range__<T>::closed_interval(value, left, right) : range__<T>::closed_interval(value, right, left);
}

template <typename T>
constexpr bool IsInLeftOpenRange(const T &value, const T &left, const T &right) {
    return (left < right) ? range__<T>::left_open_interval(value, left, right) : range__<T>::right_open_interval(value, right, left);
}

template <typename T>
constexpr bool IsInRightOpenRange(const T &value, const T &left, const T &right) {
    return (left < right) ? range__<T>::right_open_interval(value, left, right) : range__<T>::left_open_interval(value, right, left);
}
