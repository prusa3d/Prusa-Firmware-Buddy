//display_helper.h
#pragma once

#include "guitypes.hpp"
#include "Rect16.h"
#include "align.hpp"
#include "../../lang/string_view_utf8.hpp"

enum class is_multiline : bool { no,
    yes };

struct text_flags {
    Align_t align;
    uint8_t font_flg : 7;
    is_multiline multiline : 1;
    text_flags(Align_t align, uint8_t font_flg = 0, is_multiline multiline = is_multiline::no)
        : align(align)
        , font_flg(font_flg)
        , multiline(multiline) {}
    text_flags(Align_t align, is_multiline multiline)
        : align(align)
        , font_flg(0)
        , multiline(multiline) {}
};

struct icon_flags {
    Align_t align;
    uint8_t raster_flags;
    icon_flags(Align_t align, uint8_t raster_flags = 0)
        : align(align)
        , raster_flags(raster_flags) {}
};

extern size_ui16_t render_text_singleline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg);
extern void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, text_flags flags);
extern void render_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, icon_flags flags);
extern void render_unswapable_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, icon_flags flags);

extern point_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars); // rewinds text to begin
//extern int font_line_chars(const font_t *pf, string_view_utf8 str, uint16_t line_width);
extern void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, color_t color);
