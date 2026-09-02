/// @file
#pragma once

#include <optional>

#include <fonts.hpp>
#include <font_flags.hpp>
#include <Rect16.h>
#include <string_view_utf8.hpp>

struct AutoSelectFontArgs {
    string_view_utf8 text;
    Rect16 rect;
    Font largest;
    Font smallest;
    bool multiline;
};

/// @returns the largest font that can fit @p text into @p rect
std::optional<Font> auto_select_font(const AutoSelectFontArgs &args);

/// A standard method of indicating a color
void draw_color_swatch(Rect16 rect, Align_t align, OptionalColor color, Color color_back);
