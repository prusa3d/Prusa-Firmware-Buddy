/**
 * @file align.hpp
 * @author Radek Vana
 * @brief alignment class
 * @date 2021-01-31
 */
#pragma once

namespace {
#include "align_c.h"
};

class Align_t {
    uint8_t data;
    constexpr Align_t(uint8_t data)
        : data(data) {}; // use one of factory methods instead
public:
    enum class vertical {
        top = ALIGN_TOP,
        center = ALIGN_VCENTER,
        bottom = ALIGN_BOTTOM
    };

    enum class horizontal {
        left = ALIGN_LEFT,
        center = ALIGN_HCENTER,
        right = ALIGN_RIGHT
    };

    // public ctors
    constexpr Align_t(vertical v, horizontal h = horizontal::left)
        : data(uint8_t(v) | uint8_t(h)) {};
    constexpr Align_t(horizontal h, vertical v = vertical::top)
        : data(uint8_t(v) | uint8_t(h)) {};

    // factory methods
    constexpr static Align_t Left() { return Align_t(ALIGN_LEFT); }
    constexpr static Align_t Right() { return Align_t(ALIGN_RIGHT); }
    constexpr static Align_t Top() { return Align_t(ALIGN_TOP); }
    constexpr static Align_t Bottom() { return Align_t(ALIGN_BOTTOM); }

    constexpr static Align_t Center() { return Align_t(ALIGN_CENTER); }
    constexpr static Align_t CenterTop() { return Align_t(ALIGN_CENTER_TOP); }
    constexpr static Align_t CenterBottom() { return Align_t(ALIGN_CENTER_BOTTOM); }

    constexpr static Align_t LeftTop() { return Align_t(ALIGN_LEFT_TOP); }
    constexpr static Align_t LeftCenter() { return Align_t(ALIGN_LEFT_CENTER); }
    constexpr static Align_t LeftBottom() { return Align_t(ALIGN_LEFT_BOTTOM); }

    constexpr static Align_t RightTop() { return Align_t(ALIGN_RIGHT_TOP); }
    constexpr static Align_t RightCenter() { return Align_t(ALIGN_RIGHT_CENTER); }
    constexpr static Align_t RightBottom() { return Align_t(ALIGN_RIGHT_BOTTOM); }

    // getter methods
    constexpr vertical Vertical() const { return vertical(data & ALIGN_VMASK); }
    constexpr horizontal Horizontal() const { return horizontal(data & ALIGN_HMASK); }
};

static_assert(sizeof(Align_t) == sizeof(uint8_t), "error Align_t must have size as uint8_t");
