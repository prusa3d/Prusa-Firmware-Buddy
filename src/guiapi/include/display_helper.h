//display_helper.h
#pragma once

#include "guitypes.hpp"
#include "Rect16.h"
#include "align.hpp"
#include "font_flags.hpp"
#include "raster_opfn.hpp"
#include "../../lang/string_view_utf8.hpp"

extern size_ui16_t render_text_singleline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg);
extern void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, text_flags flags);
extern void render_icon_align(Rect16 rc, uint16_t id_res, color_t clr_back, icon_flags flags);

extern size_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars); // rewinds text to begin
//extern int font_line_chars(const font_t *pf, string_view_utf8 str, uint16_t line_width);
extern void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, color_t color);

extern void render_rect(Rect16 rc, color_t clr); // to prevent dirrect access to display
