//display_helper.h
#pragma once

#include "window.hpp"
#include "guitypes.hpp"
#include "../../lang/string_view_utf8.hpp"

static const uint16_t RENDER_FLG_ALIGN = 0x00ff; // alignment mask (ALIGN_xxx)
static const uint16_t RENDER_FLG_ROPFN = 0x0f00; // raster operation function mask (ROPFN_xxx << 8)
static const uint16_t RENDER_FLG_WORDB = 0x1000; // multiline text
#define RENDER_FLG(a, r) (a | r << 8)            // render flag macro (ALIGN and ROPFN)

extern size_ui16_t render_text(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg, uint16_t flags);
/// FIXME add \param flags documentation
extern void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags);
extern void render_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, uint16_t flags);
extern void render_unswapable_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, uint16_t flags);

extern point_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars); // rewinds text to begin
//extern int font_line_chars(const font_t *pf, string_view_utf8 str, uint16_t line_width);
extern void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, color_t color);
