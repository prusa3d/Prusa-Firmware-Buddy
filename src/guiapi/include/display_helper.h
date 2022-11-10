/**
 * @file display_helper.h
 * @brief functions designed to be directly used in GUI
 */
#pragma once

#include "guitypes.hpp"
#include "Rect16.h"
#include "align.hpp"
#include "font_flags.hpp"
#include "raster_opfn.hpp"
#include "../../lang/string_view_utf8.hpp"

void get_char_position_in_font(unichar c, const font_t *pf, uint8_t *charX, uint8_t *charY);
extern size_ui16_t render_text_singleline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg);
extern void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, text_flags flags, bool fill_rect = true);
extern void render_icon_align(Rect16 rc, const png::Resource *res, color_t clr_back, icon_flags flags);

extern size_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars); // rewinds text to begin
//extern int font_line_chars(const font_t *pf, string_view_utf8 str, uint16_t line_width);
extern void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, color_t color);

extern void render_rect(Rect16 rc, color_t clr); // to prevent direct access to display
