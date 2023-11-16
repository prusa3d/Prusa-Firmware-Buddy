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

    constexpr color_scheme(color_t normal, color_t focused, color_t shadowed, color_t focused_and_shadowed)
        : normal(normal)
        , focused(focused)
        , shadowed(shadowed)
        , focused_and_shadowed(focused_and_shadowed) {}

    constexpr color_scheme(color_t clr)
        : color_scheme(clr, clr, clr, clr) {}

    constexpr color_t Get(bool is_focused, bool is_shadowed) const {
        if ((is_focused) && (is_shadowed)) {
            return focused_and_shadowed;
        }
        if ((is_focused) && (!is_shadowed)) {
            return focused;
        }
        if ((!is_focused) && (is_shadowed)) {
            return shadowed;
        }
        // if ((!is_focused) && (!is_shadowed))
        return normal;
    }

    constexpr bool operator==(const color_scheme &other) const {
        return (normal == other.normal) && (focused == other.focused) && (shadowed == other.shadowed) && (focused_and_shadowed == other.focused_and_shadowed);
    }
    constexpr bool operator!=(const color_scheme &other) const {
        return !((*this) == other);
    }
};
