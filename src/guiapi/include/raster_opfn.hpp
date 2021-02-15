/**
 * @file raster_opfn.hpp
 * @author Radek Vana
 * @brief raster operation function constants for C
 * @date 2021-02-02
 */
#pragma once

#include "align.hpp"
#include <stdint.h>

namespace {
#include "raster_opfn_c.h"
};

enum class is_inverted : bool {
    no,
    yes
};

enum class has_swapped_bw : bool {
    no,
    yes
};

enum class is_disabled : bool {
    no,
    yes
};

struct ropfn {
    is_inverted invert : 1;
    has_swapped_bw swap_bw : 1;
    is_disabled disable : 1;
    constexpr ropfn(is_inverted invert = is_inverted::no, has_swapped_bw swap_bw = has_swapped_bw::no, is_disabled disable = is_disabled::no)
        : invert(invert)
        , swap_bw(swap_bw)
        , disable(disable) {}

    constexpr uint8_t ConvertToC() const {
        uint8_t ret = 0;
        if (invert == is_inverted::yes)
            ret |= ROPFN_INVERT;
        if (swap_bw == has_swapped_bw::yes)
            ret |= ROPFN_SWAPBW;
        if (disable == is_disabled::yes)
            ret |= ROPFN_DISABLE;
        return ret;
    }
};

struct icon_flags {
    Align_t align;
    ropfn raster_flags;
    constexpr icon_flags(Align_t align, ropfn rop = ropfn())
        : align(align)
        , raster_flags(rop) {}

    constexpr bool IsInverted() { return raster_flags.invert == is_inverted::yes; }
    constexpr bool HasSwappedBW() { return raster_flags.swap_bw == has_swapped_bw::yes; }
    constexpr bool IsDisabled() { return raster_flags.disable == is_disabled::yes; }
};
