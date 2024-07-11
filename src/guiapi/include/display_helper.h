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
#include "fonts.hpp"

uint32_t get_char_position_in_font(unichar c, const font_t *pf);
size_ui16_t render_text_singleline(Rect16 rc, StringReaderUtf8 &reader, const font_t *pf, Color clr_bg, Color clr_fg);
void render_text_align(Rect16 rc, const string_view_utf8 &text, Font, Color clr0, Color clr1, padding_ui8_t padding, text_flags flags, bool fill_rect = true);
void render_icon_align(Rect16 rc, const img::Resource *res, Color clr_back, icon_flags flags);

std::optional<size_ui16_t> characters_meas_text(const string_view_utf8 &str, uint16_t max_chars_per_line, uint16_t *numOfUTF8Chars = nullptr); // rewinds text to begin
size_ui16_t font_meas_text(Font, const string_view_utf8 &str, uint16_t *numOfUTF8Chars); // rewinds text to begin
void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, Color color);

void render_rect(Rect16 rc, Color clr); // to prevent direct access to display

// private only
void render_rounded_rect(Rect16 rc, Color bg_clr, Color fg_clr, uint8_t rad, uint8_t flag);
