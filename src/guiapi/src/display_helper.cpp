//display_helper.cpp

#include <algorithm>

#include "display_helper.h"
#include "display.h"
#include "gui_timer.h"
#include "window.hpp"
#include "gui.hpp"
#include "../lang/string_view_utf8.hpp"
#include "../lang/unaccent.hpp"
#include "../common/str_utils.hpp"
#include "ScreenHandler.hpp"
#include "guitypes.hpp"
#include "cmath_ext.h"

//#define UNACCENT

#ifdef UNACCENT
std::pair<const char *, uint8_t> ConvertUnicharToFontCharIndex(unichar c) {
    // for now we have a translation table and in the future we'll have letters with diacritics too (i.e. more font bitmaps)
    const auto &a = UnaccentTable::Utf8RemoveAccents(c);
    return std::make_pair(a.str, a.size); // we are returning some number of characters to replace the input utf8 character
}
#endif

/// Fill space from [@top, @left] corner to the end of @rc with height @h
/// If @h is too high, it will be cropped so nothing is drawn outside of the @rc but
/// @top and @left are not checked whether they are in @rc
void fill_till_end_of_line(const int left, const int top, const int h, Rect16 rc, color_t clr) {
    display::FillRect(Rect16(left, top, std::max(0, rc.EndPoint().x - left), CLAMP(rc.EndPoint().y - top, 0, h)), clr);
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
size_ui16_t render_text_singleline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.Left();
    int y = rc.Top();

    const int w = pf->w; //char width
    const int h = pf->h; //char height
    // prepare for stream processing
    unichar c = 0;
    no_wrap text_plain;

    while (true) {
        c = text_plain.character(str);

        if (c == 0)
            break;

        /// Break line char or drawable char won't fit into this line any more
        if (c == '\n') {
            break; /// end of single line => no more text to print
        }

        if (x + w > rc.EndPoint().x) {
            continue;
        }

        /// draw part
#ifdef UNACCENT
        // FIXME no check for enough space to draw char/chars
        if (c < 128) {
            display::DrawChar(point_ui16(x, y), c, pf, clr_bg, clr_fg);
            x += w;
        } else {
            auto convertedChar = ConvertUnicharToFontCharIndex(c);
            for (size_t i = 0; i < convertedChar.second; ++i) {
                display::DrawChar(point_ui16(x, y), convertedChar.first[i], pf, clr_bg, clr_fg);
                x += w; // this will screw up character counting for DE language @@TODO
            }
        }
#else
        display::DrawChar(point_ui16(x, y), c, pf, clr_bg, clr_fg);
        x += w;
#endif
    }
    /// fill background to the end of the line and all below till the border of @rc
    fill_till_end_of_line(x, y, h, rc, clr_bg);
    y += h;
    int h1 = std::max(0, rc.EndPoint().y - y);
    if (h1 > 0) /// FIXME hotfix because FillRect draws nonempty rect. for height 0
        display::FillRect(Rect16(rc.Left(), y, rc.Width(), h1), clr_bg);

    return size_ui16_t { rc.Width(), rc.Height() };
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
size_ui16_t render_text_multiline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.Left();
    int y = rc.Top();

    const int w = pf->w; //char width
    const int h = pf->h; //char height
    // prepare for stream processing
    unichar c = 0;
    /// TODO define parent class for both below and use parent.character(str) instead (few lines below)
    text_wrapper<ram_buffer, const font_t *> wrapper(rc.Width(), pf);

    while (true) {
        c = wrapper.character(str);

        if (c == 0)
            break;

        /// Break line char or drawable char won't fit into this line any more
        if (c == '\n') {
            /// draw background till the border of @rc
            fill_till_end_of_line(x, y, h, rc, clr_bg);
            /// new line
            y += h;
            x = rc.Left();

            if (y + h > rc.EndPoint().y) /// next char won't fit vertically
                break;

            continue;
        }

        if (x + w > rc.EndPoint().x) {
            continue;
        }

        /// draw part
#ifdef UNACCENT
        // FIXME no check for enough space to draw char/chars
        if (c < 128) {
            display::DrawChar(point_ui16(x, y), c, pf, clr_bg, clr_fg);
            x += w;
        } else {
            auto convertedChar = ConvertUnicharToFontCharIndex(c);
            for (size_t i = 0; i < convertedChar.second; ++i) {
                display::DrawChar(point_ui16(x, y), convertedChar.first[i], pf, clr_bg, clr_fg);
                x += w; // this will screw up character counting for DE language @@TODO
            }
        }
#else
        display::DrawChar(point_ui16(x, y), c, pf, clr_bg, clr_fg);
        x += w;
#endif
    }
    /// fill background to the end of the line and all below till the border of @rc
    fill_till_end_of_line(x, y, h, rc, clr_bg);
    y += h;
    int h1 = std::max(0, rc.EndPoint().y - y);
    if (h1 > 0) /// FIXME hotfix because FillRect draws nonempty rect. for height 0
        display::FillRect(Rect16(rc.Left(), y, rc.Width(), h1), clr_bg);

    return size_ui16_t { rc.Width(), rc.Height() };
}

/// Fills space between two rectangles with a color
/// @r_in must be completely in @r_out
void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, color_t color) {
    if (!r_out->Contain(*r_in))
        return;
    /// top
    const Rect16 rc_t = { r_out->Left(), r_out->Top(), r_out->Width(), uint16_t(r_in->Top() - r_out->Top()) };
    display::FillRect(rc_t, color);
    /// bottom
    const Rect16 rc_b = { r_out->Left(), int16_t(r_in->Top() + r_in->Height()), r_out->Width(), uint16_t((r_out->Top() + r_out->Height()) - (r_in->Top() + r_in->Height())) };
    display::FillRect(rc_b, color);
    /// left
    const Rect16 rc_l = { r_out->Left(), r_in->Top(), uint16_t(r_in->Left() - r_out->Left()), r_in->Height() };
    display::FillRect(rc_l, color);
    /// right
    const Rect16 rc_r = { int16_t(r_in->Left() + r_in->Width()), r_in->Top(), uint16_t((r_out->Left() + r_out->Width()) - (r_in->Left() + r_in->Width())), r_in->Height() };
    display::FillRect(rc_r, color);
}

/// Draws text into the specified rectangle with proper alignment (@flags)
/// This cannot horizontally align a text spread over more lines (multiline text).
/// \param flags Use ALIGN constants from guitypes.h. Use RENDER_FLG_WORDB for line wrapping
void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags) {
    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);

    /// 1st pass reading the string_view_utf8 - font_meas_text also computes the number of utf8 characters (i.e. individual bitmaps) in the input string
    uint16_t strlen_text = 0;
    const point_ui16_t txt_size = font_meas_text(font, &text, &strlen_text);
    if (txt_size.x == 0 || txt_size.y == 0) {
        /// empty text => draw background rectangle only
        display::FillRect(rc, clr0);
        return;
    }

    /// single line, can modify rc pad
    if (font->h * 2 > rc_pad.Height()                              /// 2 lines would not fit
        || (txt_size.y == font->h && txt_size.x <= rc_pad.Width()) /// text fits into a single line completely
        || !(flags & RENDER_FLG_WORDB)) {                          /// wrapping turned off

        Rect16 rc_txt = Rect16(0, 0, txt_size.x, txt_size.y); /// set size
        rc_txt.Align(rc_pad, flags & ALIGN_MASK);             /// position the rectangle
        rc_pad = rc_txt.Intersection(rc_pad);                 ///  set padding rect to new value, crop the rectangle if the text is too long

        /// 2nd pass reading the string_view_utf8 - draw the text
        render_text_singleline(rc_pad, text, font, clr0, clr1);
    } else {
        /// multiline text
        /// 2nd pass reading the string_view_utf8 - draw the text
        render_text_multiline(rc_pad, text, font, clr0, clr1);
    }

    /// fill borders (padding)
    fill_between_rectangles(&rc, &rc_pad, clr0);
}

void render_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, uint16_t flags) {
    color_t opt_clr;
    switch ((flags >> 8) & (ROPFN_SWAPBW | ROPFN_DISABLE)) {
    case ROPFN_SWAPBW | ROPFN_DISABLE:
        opt_clr = GuiDefaults::ColorDisabled;
        break;
    case ROPFN_SWAPBW:
        opt_clr = clr0 ^ 0xffffffff;
        break;
    case ROPFN_DISABLE:
        opt_clr = clr0;
        break;
    default:
        opt_clr = clr0;
        break;
    }
    point_ui16_t wh_ico = icon_meas(resource_ptr(id_res));
    if (wh_ico.x && wh_ico.y) {
        Rect16 rc_ico = Rect16(0, 0, wh_ico.x, wh_ico.y);
        rc_ico.Align(rc, flags & ALIGN_MASK);
        rc_ico = rc_ico.Intersection(rc);
        fill_between_rectangles(&rc, &rc_ico, opt_clr);
        display::DrawIcon(point_ui16(rc_ico.Left(), rc_ico.Top()), id_res, clr0, (flags >> 8) & 0x0f);
    } else
        display::FillRect(rc, opt_clr);
}

//todo rewrite
void render_unswapable_icon_align(Rect16 rc, uint16_t id_res, color_t clr0, uint16_t flags) {
    color_t opt_clr;
    switch ((flags >> 8) & (ROPFN_SWAPBW | ROPFN_DISABLE)) {
    case ROPFN_SWAPBW | ROPFN_DISABLE:
        opt_clr = GuiDefaults::ColorDisabled;
        break;
    case ROPFN_SWAPBW:
        opt_clr = clr0 ^ 0xffffffff;
        break;
    case ROPFN_DISABLE:
        opt_clr = clr0;
        break;
    default:
        opt_clr = clr0;
        break;
    }
    flags &= ~(ROPFN_SWAPBW << 8);
    point_ui16_t wh_ico = icon_meas(resource_ptr(id_res));
    if (wh_ico.x && wh_ico.y) {
        Rect16 rc_ico = Rect16(0, 0, wh_ico.x, wh_ico.y);
        rc_ico.Align(rc, flags & ALIGN_MASK);
        rc_ico = rc_ico.Intersection(rc);
        fill_between_rectangles(&rc, &rc_ico, opt_clr);
        display::DrawIcon(point_ui16(rc_ico.Left(), rc_ico.Top()), id_res, clr0, (flags >> 8) & 0x0f);
    } else
        display::FillRect(rc, opt_clr);
}

point_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars) {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    const int8_t char_w = pf->w;
    const int8_t char_h = pf->h;
    *numOfUTF8Chars = 0;
    unichar c = 0;
    while ((c = str->getUtf8Char()) != 0) {
        ++(*numOfUTF8Chars);
        if (c == '\n') {
            if (x + char_w > w)
                w = x + char_w;
            y += char_h;
            x = 0;
        } else
            x += char_w;
        h = y + char_h;
    }
    str->rewind();
    return point_ui16((uint16_t)std::max(x, w), (uint16_t)h);
}
