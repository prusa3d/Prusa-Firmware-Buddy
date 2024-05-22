/**
 * @file font_flags.hpp
 * @author Radek Vana
 * @brief definition of font render flags
 * @date 2021-02-02
 */
#pragma once

#include "align.hpp"
#include <stdint.h>

enum class is_multiline : bool { no,
    yes };

struct text_flags {
    Align_t align;
    is_multiline multiline;

    text_flags(Align_t align, is_multiline multiline = is_multiline::no)
        : align(align)
        , multiline(multiline) {}

    bool IsMultiline() { return multiline == is_multiline::yes; }
};
