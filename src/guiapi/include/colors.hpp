/**
 * @file colors.hpp
 * @author Radek Vana
 * @brief color definitions
 * @date 2021-01-30
 */

#pragma once

enum class color_t : uint32_t {
    Black = 0x00000000L,
    White = 0x00ffffffL,
    Red = 0x000000ffL,
    RedAlert = 0x002646e7L,
    Lime = 0x0000ff00L,
    Blue = 0x00ff0000L,
    Yellow = 0x0000ffffL,
    Cyan = 0x00ffff00L,
    Magenta = 0x00ff00ffL,
    Silver = 0x00c0c0c0L,
    Gray = 0x00808080L,
    DarkGray = 0x005B5B5BL,
    Maroon = 0x00000080L,
    Olive = 0x00008080L,
    Green = 0x00008000L,
    Purple = 0x00800080L,
    Teal = 0x00808000L,
    Navy = 0x00800000L,
    Orange = 0x001B65F8L,
    DarkKhaki = 0x006BD7DBL
};
