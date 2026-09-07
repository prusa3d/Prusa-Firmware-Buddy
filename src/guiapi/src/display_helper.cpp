/**
 * @file display_helper.cpp
 */

#include <algorithm>

#include "display_helper.h"
#include "display.hpp"
#include "window.hpp"
#include "gui.hpp"
#include "../lang/string_view_utf8.hpp"
#include "ScreenHandler.hpp"
#include <math.h>
#include "guitypes.hpp"
#include "cmath_ext.h"
#include <bsod/bsod.h>

/// Fill space from [@top, @left] corner to the end of @rc with height @h
/// If @h is too high, it will be cropped so nothing is drawn outside of the @rc but
/// @top and @left are not checked whether they are in @rc
void fill_till_end_of_line(const int left, const int top, const int h, Rect16 rc, Color clr) {
    display::fill_rect(Rect16(left, top, std::max(0, rc.EndPoint().x - left), CLAMP(rc.EndPoint().y - top, 0, h)), clr);
}

/// Fills space between two rectangles with a color
/// @r_in must be completely in @r_out
void fill_between_rectangles(const Rect16 *r_out, const Rect16 *r_in, Color color) {
    if (!r_out->Contain(*r_in)) {
        return;
    }
    /// top
    const Rect16 rc_t = { r_out->Left(), r_out->Top(), r_out->Width(), uint16_t(r_in->Top() - r_out->Top()) };
    display::fill_rect(rc_t, color);
    /// bottom
    const Rect16 rc_b = { r_out->Left(), int16_t(r_in->Top() + r_in->Height()), r_out->Width(), uint16_t((r_out->Top() + r_out->Height()) - (r_in->Top() + r_in->Height())) };
    display::fill_rect(rc_b, color);
    /// left
    const Rect16 rc_l = { r_out->Left(), r_in->Top(), uint16_t(r_in->Left() - r_out->Left()), r_in->Height() };
    display::fill_rect(rc_l, color);
    /// right
    const Rect16 rc_r = { int16_t(r_in->Left() + r_in->Width()), r_in->Top(), uint16_t((r_out->Left() + r_out->Width()) - (r_in->Left() + r_in->Width())), r_in->Height() };
    display::fill_rect(rc_r, color);
}

size_ui16_t calculate_text_size(const string_view_utf8 &str, const Font font, is_multiline multiline) {
    const auto *pf = resource_font(font);
    StringReaderUtf8 reader(str);
    const auto layout = RectTextLayout(reader, 255, 255, multiline);
    debug_assert(!layout.has_text_overflown());
    return size_ui16_t(layout.get_width_in_chars() * pf->w, layout.get_height_in_chars() * pf->h);
}

void render_line(StringReaderUtf8 &reader, uint8_t chars_to_print, Rect16 rc, const font_t *pf, Color clr_bg, Color clr_fg) {
    const uint16_t buff_char_capacity = display::buffer_pixel_size() / (pf->w * pf->h);
    debug_assert(buff_char_capacity > 0 && "Buffer needs to take at least one character");
    point_ui16_t pt = point_ui16(rc.Left(), rc.Top());

    uint8_t chars_left = chars_to_print;
    while (chars_left > 0) {
        const uint8_t char_cnt = std::min(static_cast<uint16_t>(chars_left), buff_char_capacity);
        // Storing text in the display buffer
        // It has to know how many chars will be stored to correctly compute display buffer offsets
        for (uint8_t j = 0; j < char_cnt; j++) {
            const unichar c = reader.getUtf8Char();
            if (c == '\n' || c == '\0') {
                bsod("Bad RectTextLayout");
            }
            display::store_char_in_buffer(char_cnt, j, c, pf, clr_bg, clr_fg);
        }
        // Drawing from the buffer
        chars_left -= char_cnt;
        display::draw_from_buffer(pt, char_cnt * pf->w, pf->h);
        pt.x += char_cnt * pf->w;
    }
}

void render_text_align(Rect16 rc, const string_view_utf8 &text, const Font f, Color clr_bg, Color clr_fg, padding_ui8_t padding, text_flags flags, bool fill_rect) {
    StringReaderUtf8 reader(text);
    render_text_align(rc, reader, f, clr_bg, clr_fg, padding, flags, fill_rect);
}

void render_text_align(Rect16 rc, StringReaderUtf8 &reader, const Font f, Color clr_bg, Color clr_fg, padding_ui8_t padding, text_flags flags, bool fill_rect) {
    const font_t *font = resource_font(f);
    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);

    auto reader_copy = reader.copy();
    const RectTextLayout layout = RectTextLayout(reader_copy, rc_pad.Width() / font->w, rc_pad.Height() / font->h, flags.multiline);

    debug_assert(flags.overflow == check_overflow::no || !layout.has_text_overflown());

    if (layout.get_width_in_chars() == 0 || layout.get_height_in_chars() == 0) {
        if (fill_rect) {
            display::fill_rect(rc, clr_bg);
        }
        return;
    }

    Rect16 rc_txt = Rect16(0, 0, layout.get_width_in_chars() * font->w, layout.get_height_in_chars() * font->h);
    rc_txt.Align(rc_pad, flags.align);
    rc_pad = rc_txt.Intersection(rc_pad); ///  set padding rect to new value, crop the rectangle if the text is too long

    for (size_t i = 0; i < layout.get_height_in_chars(); ++i) {
        Rect16 rect_to_align(rc_pad.Left(), rc_pad.Top() + i * font->h, rc_pad.Width(), font->h);
        const size_t line_char_cnt = layout.get_line_characters(i);
        Rect16 line_rect(0, 0, font->w * line_char_cnt, font->h);
        line_rect.Align(rect_to_align, flags.align);

        // in front of line
        const Rect16 front = rect_to_align.LeftSubrect(line_rect);
        if (front.Width()) {
            display::fill_rect(front, clr_bg);
        }
        // behind line
        const Rect16 behind = rect_to_align.RightSubrect(line_rect);
        if (behind.Width()) {
            display::fill_rect(behind, clr_bg);
        }

        render_line(reader, line_char_cnt, line_rect, font, clr_bg, clr_fg);

        // skip character, that splits the lines (usually '\n' || ' ')
        if (layout.get_skip_char_on_line(i)) {
            reader.skip(1);
        }
    }

    /// fill borders (padding)
    if (fill_rect) {
        fill_between_rectangles(&rc, &rc_pad, clr_bg);
    }
}

void render_icon_align(Rect16 rc, const img::Resource *res, Color clr_back, icon_flags flags) {

    if (res) {
        point_ui16_t wh_ico = { res->w, res->h };
        Rect16 rc_ico = Rect16(0, 0, wh_ico.x, wh_ico.y);
        rc_ico.Align(rc, flags.align);
        rc_ico = rc_ico.Intersection(rc);
        display::draw_img(point_ui16(rc_ico.Left(), rc_ico.Top()), *res, clr_back, flags.raster_flags);
    } else {
        display::fill_rect(rc, clr_back);
    }
}

void render_rect(Rect16 rc, Color clr) {
    display::fill_rect(rc, clr);
}

void render_rounded_rect(Rect16 rc, Color bg_clr, Color fg_clr, uint8_t rad, uint8_t flag) {
    display::draw_rounded_rect(rc, bg_clr, fg_clr, rad, flag);
}
