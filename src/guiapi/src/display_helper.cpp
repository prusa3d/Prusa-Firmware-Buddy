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

void draw_char_and_increment(const font_t *pf, color_t clr_bg, color_t clr_fg, unichar c, int &ref_x, int y, int w) {
    // FIXME no check for enough space to draw char/chars
    if (c < 128) {
        display::DrawChar(point_ui16(ref_x, y), c, pf, clr_bg, clr_fg);
        ref_x += w;
    } else {
        auto convertedChar = ConvertUnicharToFontCharIndex(c);
        for (size_t i = 0; i < convertedChar.second; ++i) {
            display::DrawChar(point_ui16(ref_x, y), convertedChar.first[i], pf, clr_bg, clr_fg);
            ref_x += w; // this will screw up character counting for DE language @@TODO
        }
    }
}

#else // !UNACCENT

void draw_char_and_increment(const font_t *pf, color_t clr_bg, color_t clr_fg, unichar c, int &ref_x, int y, int w) {
    display::DrawChar(point_ui16(ref_x, y), c, pf, clr_bg, clr_fg);
    ref_x += w;
}

#endif

/// Fill space from [@top, @left] corner to the end of @rc with height @h
/// If @h is too high, it will be cropped so nothing is drawn outside of the @rc but
/// @top and @left are not checked whether they are in @rc
void fill_till_end_of_line(const int left, const int top, const int h, Rect16 rc, color_t clr) {
    display::FillRect(Rect16(left, top, std::max(0, rc.EndPoint().x - left), CLAMP(rc.EndPoint().y - top, 0, h)), clr);
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

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
template <class T>
size_ui16_t render_line(T &textWrapper, Rect16 rc, string_view_utf8 &str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    int x = rc.Left();
    int y = rc.Top();
    size_t drawn_chars = 0;

    const int w = pf->w; //char width

    // prepare for stream processing
    unichar c = 0;

    while (true) {
        c = textWrapper.character(str);

        if (c == 0)
            break;

        /// Break line char or drawable char won't fit into this line any more
        if (c == '\n') {
            break; /// end of single line => no more text to print
        }

        if (x + w > rc.EndPoint().x) {
            break;
        }

        /// draw part
        draw_char_and_increment(pf, clr_bg, clr_fg, c, x, y, w);
        ++drawn_chars;
    }

    return size_ui16_t { uint16_t(drawn_chars * w), rc.Height() };
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
size_ui16_t render_text_singleline(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    no_wrap text_plain;

    return render_line(text_plain, rc, str, pf, clr_bg, clr_fg);
}

//count characters in lines
static RectTextLayout multiline_loop(uint8_t MaxColsInRect, uint8_t MaxRowsInRect, string_view_utf8 str) {
    RectTextLayout layout;

    // prepare for stream processing
    unichar c = 0;

    // text_wrapper needs font to support Disproportionate fonts in future
    // multiline_loop is not compatible with them and will need to be rewritten
    // so I can use dummy font
    static constexpr font_emulation_w1 dummy;
    text_wrapper<ram_buffer, const font_emulation_w1 *> wrapper(MaxColsInRect, &dummy);

    bool exit = false;

    while (!exit) {
        c = wrapper.character(str);

        switch (c) {
        /// Break line char or drawable char won't fit into this line any more
        case '\n':
            /// new line
            if (!layout.NewLine()) { /// next char won't fit vertically
                exit = true;
            }
            break;

        case 0:
            exit = true;
            break;

        default:
            //there are no texts longer than line, checked by content
            layout.IncrementNumOfCharsUpTo(MaxColsInRect); //text_wrapper should not let this fail
        }
    }

    return layout;
}

/// Draws text into the specified rectangle with proper alignment (@flags)
/// This cannot horizontally align a text spread over more lines (multiline text).
void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr_bg, color_t clr_fg, padding_ui8_t padding, text_flags flags) {
    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);

    /// 1st pass reading the string_view_utf8 - font_meas_text also computes the number of utf8 characters (i.e. individual bitmaps) in the input string
    uint16_t strlen_text = 0;
    const size_ui16_t txt_size = font_meas_text(font, &text, &strlen_text);
    if (txt_size.w == 0 || txt_size.h == 0) {
        /// empty text => draw background rectangle only
        display::FillRect(rc, clr_bg);
        return;
    }

    /// single line, can modify rc pad
    if (font->h * 2 > rc_pad.Height()                              /// 2 lines would not fit
        || (txt_size.h == font->h && txt_size.w <= rc_pad.Width()) /// text fits into a single line completely
        || !flags.IsMultiline()) {                                 /// wrapping turned off

        Rect16 rc_txt = Rect16(0, 0, txt_size.w, txt_size.h); /// set size
        rc_txt.Align(rc_pad, flags.align);                    /// position the rectangle
        rc_pad = rc_txt.Intersection(rc_pad);                 ///  set padding rect to new value, crop the rectangle if the text is too long

        /// 2nd pass reading the string_view_utf8 - draw the text
        /// surrounding of rc_pad will be printed with back color
        rc_pad = Rect16(rc_pad.TopLeft(), render_text_singleline(rc_pad, text, font, clr_bg, clr_fg));
    } else {
        /// multiline text
        const uint8_t MaxColsInRect = std::min(255, rc_pad.Width() / font->w);
        const uint8_t MaxRowsInRect = std::min(255, rc_pad.Height() / font->h);

        /// 2nd pass reading the string_view_utf8 - draw the text
        RectTextLayout layout = multiline_loop(MaxColsInRect, MaxRowsInRect, text);

        Rect16 rc_txt = Rect16(0, 0, rc_pad.Width(), font->h * layout.GetLineCount()); /// set size
        rc_txt.Align(rc_pad, flags.align);                                             /// position the rectangle
        rc_pad = rc_txt.Intersection(rc_pad);                                          ///  set padding rect to new value, crop the rectangle if the text is too long

        Rect16 line_to_align = rc_pad;
        line_to_align = Rect16::Height_t(font->h); //helps with calculations

        /// 3rd pass reading the string_view_utf8 - draw the text
        text.rewind();
        text_wrapper<ram_buffer, const font_t *> wrapper(rc_pad.Width(), font);
        for (size_t i = 0; i < layout.GetLineCount(); ++i) {
            const size_t line_char_cnt = layout.LineCharacters(i);
            Rect16 line_rect(0, 0, font->w * line_char_cnt, font->h);
            line_rect.Align(line_to_align, flags.align);

            //in front of line
            Rect16 front = line_to_align.LeftSubrect(line_rect);
            if (front.Width()) {
                display::FillRect(front, clr_bg);
            }
            //behind line
            Rect16 behind = line_to_align.RightSubrect(line_rect);
            if (behind.Width()) {
                display::FillRect(behind, clr_bg);
            }
            // middle of line (text)
            render_line(wrapper, line_rect, text, font, clr_bg, clr_fg);

            line_to_align += Rect16::Top_t(font->h); // next line
        }
    }

    /// fill borders (padding)
    fill_between_rectangles(&rc, &rc_pad, clr_bg);
}

void render_icon_align(Rect16 rc, uint16_t id_res, color_t clr_back, icon_flags flags) {

    point_ui16_t wh_ico = icon_meas(resource_ptr(id_res));
    if (wh_ico.x && wh_ico.y) {
        Rect16 rc_ico = Rect16(0, 0, wh_ico.x, wh_ico.y);
        rc_ico.Align(rc, flags.align);
        rc_ico = rc_ico.Intersection(rc);
        display::DrawIcon(point_ui16(rc_ico.Left(), rc_ico.Top()), id_res, clr_back, flags.raster_flags);
    } else {
        display::FillRect(rc, clr_back);
    }
}

size_ui16_t font_meas_text(const font_t *pf, string_view_utf8 *str, uint16_t *numOfUTF8Chars) {
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
    return { uint16_t(std::max(x, w)), uint16_t(h) };
}

void render_rect(Rect16 rc, color_t clr) {
    display::FillRect(rc, clr);
}
