/**
 * @file color_scheme.hpp
 * @author Radek Vana
 * @brief color scheme for windows, so they can be drawn automatically
 * @date 2021-10-06
 */
#pragma once
#include "guitypes.hpp"

struct color_scheme {
    color_t normal; // not focused and not shadowed
    color_t focused;
    color_t shadowed;
    color_t focused_and_shadowed;

    constexpr color_t Get(bool is_focused, bool is_shadowed) const {
        if (is_focused && is_shadowed) {
            return focused_and_shadowed;

        } else if (is_focused && !is_shadowed) {
            return focused;

        } else if (!is_focused && is_shadowed) {
            return shadowed;

        } else {
            return normal;
        }
    }

    constexpr bool operator==(const color_scheme &) const = default;
    constexpr bool operator!=(const color_scheme &) const = default;
};
