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

enum class is_shadowed : bool {
    no,
    yes
};

enum class is_desaturated : bool {
    no,
    yes
};

struct ropfn {
    is_inverted invert : 1;
    has_swapped_bw swap_bw : 1;
    is_shadowed shadow : 1;
    is_desaturated desatur : 1;
    constexpr ropfn(is_inverted invert = is_inverted::no, has_swapped_bw swap_bw = has_swapped_bw::no, is_shadowed shadow = is_shadowed::no, is_desaturated desatur = is_desaturated::no)
        : invert(invert)
        , swap_bw(swap_bw)
        , shadow(shadow)
        , desatur(desatur) {}

    constexpr uint8_t ConvertToC() const {
        uint8_t ret = 0;
        if (invert == is_inverted::yes) {
            ret |= ROPFN_INVERT;
        }
        if (swap_bw == has_swapped_bw::yes) {
            ret |= ROPFN_SWAPBW;
        }
        if (shadow == is_shadowed::yes) {
            ret |= ROPFN_SHADOW;
        }
        if (desatur == is_desaturated::yes) {
            ret |= ROPFN_DESATURATE;
        }

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
    constexpr bool IsShadowed() { return raster_flags.shadow == is_shadowed::yes; }
    constexpr bool IsDesaturated() { return raster_flags.desatur == is_desaturated::yes; }
};
