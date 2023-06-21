/**
 * @file font_flags.hpp
 * @author Radek Vana
 * @brief definition of font render flags
 * @date 2021-02-02
 */
#pragma once

#include "align.hpp"
#include <stdint.h>

// hidden include of C header
namespace {
#include "font_flags_c.h"
};

enum class is_multiline : bool { no,
    yes };

enum class is_swap : bool { no,
    yes };

enum class fnt_lsb : bool { no,
    yes };

struct font_flags {
    is_multiline multiline : 1;
    is_swap swap : 1;
    fnt_lsb lsb : 1;

    // used to create from font
    font_flags(uint8_t font_flg = 0, is_multiline multiline = is_multiline::no)
        : font_flags(multiline, is_swap(font_flg & FONT_FLG_SWAP), fnt_lsb(font_flg & FONT_FLG_LSBF)) {}

    font_flags(is_multiline multiline, is_swap swap = is_swap::no, fnt_lsb lsb = fnt_lsb::no)
        : multiline(multiline)
        , swap(swap)
        , lsb(lsb) {}
};

struct text_flags {
    Align_t align;
    font_flags fnt;

    text_flags(Align_t align, font_flags fnt = 0)
        : align(align)
        , fnt(fnt) {}

    bool IsMultiline() { return fnt.multiline == is_multiline::yes; }
    bool IsSwapped() { return fnt.swap == is_swap::yes; }
    bool HasLsb() { return fnt.lsb == fnt_lsb::yes; }
};
