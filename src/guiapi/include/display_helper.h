/**
 * @file display_helper.h
 * @brief functions designed to be directly used in GUI
 */
#pragma once

#include <guitypes.hpp>
#include <Rect16.h>
#include <font_flags.hpp>
#include <raster_opfn.hpp>
#include <string_view_utf8.hpp>
#include <fonts.hpp>

/**
 * @brief Calculate text size without knowing the rectangle
 * @param string                    translated string
 * @param font                      used font
 * @param multiline                 how does it handle newline (singleline has '\n' as terminated character)
 * @return size of text rect
 */
size_ui16_t calculate_text_size(const string_view_utf8 &str, const Font font, is_multiline multiline);

/**
 * @brief Renders multiline / singleline text on the display
 * @param rc                Specified rectangle
 * @param reader            reader with translated text
 * @param font              Font
 * @param clr_bg            background color
 * @param clr_fg            font/foreground color
 * @param padding           Padding for the rectangle
 * @param flags             Text flags consist of alignment and multiline
 * @param fill_rect         Redraw text surrounding rectangles
 */
void render_text_align(Rect16 rc, StringReaderUtf8 &text, const Font font, Color clr_bg, Color clr_fg, padding_ui8_t padding = padding_ui8_t(), text_flags flags = text_flags(Align_t::Left()), bool fill_rect = true);

/**
 * @brief Renders multiline / singleline text on the display
 * @param rc                Specified rectangle
 * @param text              translated text
 * @param font              Font
 * @param clr_bg            background color
 * @param clr_fg            font/foreground color
 * @param padding           Padding for the rectangle
 * @param flags             Text flags consist of alignment and multiline
 * @param fill_rect         Redraw text surrounding rectangles
 */
void render_text_align(Rect16 rc, const string_view_utf8 &text, const Font font, Color clr_bg, Color clr_fg, padding_ui8_t padding = padding_ui8_t(), text_flags flags = text_flags(Align_t::Left()), bool fill_rect = true);

void render_icon_align(Rect16 rc, const img::Resource *res, Color clr_back, icon_flags flags);

void render_rect(Rect16 rc, Color clr); // to prevent direct access to display

// private only
void render_rounded_rect(Rect16 rc, Color bg_clr, Color fg_clr, uint8_t rad, uint8_t flag);
