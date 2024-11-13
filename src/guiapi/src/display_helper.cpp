/**
 * @file display_helper.cpp
 */

#include <algorithm>

#include "display_helper.h"
#include "display.hpp"
#include "window.hpp"
#include "gui.hpp"
#include "../lang/string_view_utf8.hpp"
#include "../common/str_utils.hpp"
#include "ScreenHandler.hpp"
#include <math.h>
#include "guitypes.hpp"
#include "cmath_ext.h"
#include <utility_extensions.hpp>
#include "font_character_sets.hpp"

static constexpr const uint16_t font_full_char_indices[] = {
#include "../guiapi/include/fnt-full-indices.ipp"
};

static constexpr const uint16_t font_standard_char_indices[] = {
#include "../guiapi/include/fnt-standard-indices.ipp"
};

static constexpr const uint16_t font_digits_char_indices[] = {
#include "../guiapi/include/fnt-digits-indices.ipp"
};

bool hasASCII(FontCharacterSet charset_option) {
    return charset_option == FontCharacterSet::full || charset_option == FontCharacterSet::standard;
}

uint32_t get_char_position_in_font(unichar c, const font_t *pf) {
    if (c < uint8_t(pf->asc_min)) { // this really happens with non-utf8 characters on filesystems
        return get_char_position_in_font('?', pf);
    }

    if (hasASCII(pf->charset) && c < 127) {
        // standard ASCII character
        // This means that fonts with FontCharacterSet::full have to have all standard ASCII characters even though some of them are not used
        // We take this trade off - we waste a little bit of space, but no lower_bound is necessary for standard character indices
        return c - pf->asc_min;
    }

    // extended utf8 character - must search in the font_XXX_char_indices map

    const uint16_t *first = nullptr, *last = nullptr;
    switch (pf->charset) {
    case FontCharacterSet::full:
        first = std::begin(font_full_char_indices);
        last = std::end(font_full_char_indices);
        break;
    case FontCharacterSet::standard:
        first = std::begin(font_standard_char_indices);
        last = std::end(font_standard_char_indices);
        break;
    case FontCharacterSet::digits:
        first = std::begin(font_digits_char_indices);
        last = std::end(font_digits_char_indices);
        break;
    }

    const uint16_t *i = std::lower_bound(first, last, c);
    if (i == last || *i != c) {
        return get_char_position_in_font('?', pf);
    }
    return std::distance(first, i);
}

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

/// Draws a text into the specified rectangle @rc
/// It stores characters in buffer and then draws them all at once. Characters that doesn't fit within the rectangle are ignored.
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
template <class T>
size_ui16_t render_line(T &textWrapper, Rect16 rc, StringReaderUtf8 &reader, const font_t *pf, Color clr_bg, Color clr_fg) {
    if (!pf || pf->w == 0 || pf->h == 0 || rc.Width() < pf->w || rc.Height() < pf->h) {
        return size_ui16_t { 0, 0 };
    }

    point_ui16_t pt = point_ui16(rc.Left(), rc.Top());
    const uint16_t fnt_w = pf->w; // char width
    const uint16_t fnt_h = pf->h; // char height

    const uint16_t buff_char_capacity = display::buffer_pixel_size() / (fnt_w * fnt_h);
    assert(buff_char_capacity > 0 && "Buffer needs to take at least one character");
    uint16_t line_char_cnt = rc.Width() / fnt_w; // character count - rects are calculated through font measurings (newlines are ignored)
    uint16_t chars_cnt = 0; // character count of currently drawn loop iteration
    uint16_t chars_left = line_char_cnt; // characters left to draw

    for (uint16_t i = 0; i * buff_char_capacity < line_char_cnt; i++) {
        chars_cnt = chars_left > buff_char_capacity ? buff_char_capacity : chars_left;
        // Storing text in the display buffer
        // It has to know how many chars will be stored to correctly compute display buffer offsets
        for (uint16_t j = 0; j < chars_cnt; j++) {
            unichar c = textWrapper.character(reader);
            if (c == '\n') {
                j--; // j have to be unaffected by new line character
            } else {
                display::store_char_in_buffer(chars_cnt, j, c, pf, clr_bg, clr_fg);
            }
        }
        // Drawing from the buffer
        if (chars_cnt > 0) {
            chars_left -= chars_cnt;
            display::draw_from_buffer(pt, chars_cnt * fnt_w, fnt_h);
            pt.x += chars_cnt * fnt_w;
        }
    }

    return size_ui16(fnt_w * line_char_cnt, fnt_h);
}

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle it will be ignored
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns size of drawn area
/// Draws unused space of @rc with @clr_bg
size_ui16_t render_text_singleline(Rect16 rc, StringReaderUtf8 &reader, const font_t *pf, Color clr_bg, Color clr_fg) {
    no_wrap text_plain;
    return render_line(text_plain, rc, reader, pf, clr_bg, clr_fg);
}

// count characters in lines
static RectTextLayout multiline_loop(uint8_t MaxColsInRect, [[maybe_unused]] uint8_t MaxRowsInRect, const string_view_utf8 &str) {
    RectTextLayout layout;
    // prepare for stream processing
    unichar c = 0;

    // text_wrapper needs font to support Disproportionate fonts in future
    // multiline_loop is not compatible with them and will need to be rewritten
    // so I can use dummy font
    static constexpr font_emulation_w1 dummy;
    text_wrapper<ram_buffer, const font_emulation_w1 *> wrapper(MaxColsInRect, &dummy);

    StringReaderUtf8 reader(str);
    bool exit = false;
    while (!exit) {
        c = wrapper.character(reader);

        switch (c) {
        /// Break line char or drawable char won't fit into this line any more
        case '\n':
            /// new line
            if (!layout.NewLine() || layout.GetLineCount() >= MaxRowsInRect) { /// next char won't fit vertically
                exit = true;
            }
            break;

        case 0:
            exit = true;
            break;

        default:
            // there are no texts longer than line, checked by content
            layout.IncrementNumOfCharsUpTo(MaxColsInRect); // text_wrapper should not let this fail
        }
    }

    return layout;
}

/// Draws text into the specified rectangle with proper alignment (@flags)
/// This cannot horizontally align a text spread over more lines (multiline text).
void render_text_align(Rect16 rc, const string_view_utf8 &text, Font f, Color clr_bg, Color clr_fg, padding_ui8_t padding, text_flags flags, bool fill_rect) {
    const font_t *font = resource_font(f);
    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);

    /// 1st pass reading the string_view_utf8 - font_meas_text also computes the number of utf8 characters (i.e. individual bitmaps) in the input string
    uint16_t strlen_text = 0;
    const size_ui16_t txt_size = font_meas_text(f, text, &strlen_text);
    if (txt_size.w == 0 || txt_size.h == 0) {
        /// empty text => draw background rectangle only
        if (fill_rect) {
            display::fill_rect(rc, clr_bg);
        }
        return;
    }

    StringReaderUtf8 reader(text);
    /// single line, can modify rc pad
    if (font->h * 2 > rc_pad.Height() /// 2 lines would not fit
        || (txt_size.h == font->h && txt_size.w <= rc_pad.Width()) /// text fits into a single line completely
        || !flags.IsMultiline()) { /// wrapping turned off

        Rect16 rc_txt = Rect16(0, 0, txt_size.w, txt_size.h); /// set size
        rc_txt.Align(rc_pad, flags.align); /// position the rectangle
        rc_pad = rc_txt.Intersection(rc_pad); ///  set padding rect to new value, crop the rectangle if the text is too long

        /// 2nd pass reading the string_view_utf8 - draw the text
        /// surrounding of rc_pad will be printed with back color
        rc_pad = Rect16(rc_pad.TopLeft(), render_text_singleline(rc_pad, reader, font, clr_bg, clr_fg));
    } else {
        /// multiline text
        const uint8_t MaxColsInRect = std::min(255, rc_pad.Width() / font->w);
        const uint8_t MaxRowsInRect = std::min(255, rc_pad.Height() / font->h);

        /// 2nd pass reading the string_view_utf8 - draw the text
        RectTextLayout layout = multiline_loop(MaxColsInRect, MaxRowsInRect, text);

        Rect16 rc_txt = Rect16(0, 0, rc_pad.Width(), font->h * layout.GetLineCount()); /// set size
        rc_txt.Align(rc_pad, flags.align); /// position the rectangle
        rc_pad = rc_txt.Intersection(rc_pad); ///  set padding rect to new value, crop the rectangle if the text is too long

        Rect16 line_to_align = rc_pad;
        line_to_align = Rect16::Height_t(font->h); // helps with calculations

        /// 3rd pass reading the string_view_utf8 - draw the text
        text_wrapper<ram_buffer, const font_t *> wrapper(rc_pad.Width(), font);
        for (size_t i = 0; i < std::min(layout.GetLineCount(), MaxRowsInRect); ++i) {
            const size_t line_char_cnt = layout.LineCharacters(i);
            Rect16 line_rect(0, 0, font->w * line_char_cnt, font->h);
            line_rect.Align(line_to_align, flags.align);

            // in front of line
            Rect16 front = line_to_align.LeftSubrect(line_rect);
            if (front.Width()) {
                display::fill_rect(front, clr_bg);
            }
            // behind line
            Rect16 behind = line_to_align.RightSubrect(line_rect);
            if (behind.Width()) {
                display::fill_rect(behind, clr_bg);
            }
            // middle of line (text)
            render_line(wrapper, line_rect, reader, font, clr_bg, clr_fg);

            line_to_align += Rect16::Top_t(font->h); // next line
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

/**
 * @brief calculate size (in number of characters) of required rectangle, independent on font
 * TODO use common algorithm with draw and merge those functions in one
 *
 * @param str                           text to be measured
 * @param max_chars_per_line            max returned number of characters per line
 * @param ret_numOfUTF8Chars            optional return number of characters (use nullptr if not needed)
 * @return std::optional<size_ui16_t>   size of needed rectangle, it might be narrower than max_width
 */
std::optional<size_ui16_t> characters_meas_text(const string_view_utf8 &str, uint16_t max_chars_per_line, uint16_t *numOfUTF8Chars) {
    if (max_chars_per_line == 0) {
        return std::nullopt;
    }

    std::optional<int> last_space_index = std::nullopt;
    unichar c = 0;

    int chars_this_line = 0;
    int chars_longest_line = 0;
    int chars_tot = 0;
    int row_no = 0;

    StringReaderUtf8 reader(str);
    while ((c = reader.getUtf8Char()) != 0) {
        ++chars_tot;
        switch (c) {
        case '\n': // new line
            if (chars_this_line >= chars_longest_line) {
                chars_longest_line = chars_this_line;
            }
            ++row_no;
            chars_this_line = 0;
            last_space_index = std::nullopt;
            break;
        case ' ': // remember space position, discard multiple spaces

            // erase multiple spaces
            if (last_space_index && (*last_space_index == chars_this_line)) {
                break; // break to avoid ++chars_this_line
            }

            // erase start space
            if (chars_this_line == 0) {
                break; // break to avoid ++chars_this_line
            }

            last_space_index = chars_this_line;
            [[fallthrough]];
        default:
            if ((chars_this_line + 1) >= max_chars_per_line) {
                if (!last_space_index) {
                    return std::nullopt; // error, text does not fit
                }

                //  01234567890123
                // "Hello world!!!"
                // let's say last '!' does not fit
                // last_space_index is 5
                // chars_this_line is 13
                // chars_longest_line become last_space_index (5)
                // than we do new line
                // and set chars_this_line 13 - 5 - 1 (1 = space) + 1 (1 = newly added char)

                // chars_longest_line can become last_space_index
                chars_longest_line = std::max(chars_longest_line, *last_space_index);

                // new line
                ++row_no;

                // count chars in next line
                chars_this_line -= ((*last_space_index) + 1 - 1); // +1 space, -1 missing ++
                last_space_index = std::nullopt;

            } else {
                ++chars_this_line;
            }
        }
    }

    chars_longest_line = std::max(chars_longest_line, chars_this_line);

    if (numOfUTF8Chars) {
        *numOfUTF8Chars = chars_tot;
    }
    return size_ui16_t({ uint16_t(chars_longest_line), uint16_t(row_no + 1) });
}

size_ui16_t font_meas_text(Font font, const string_view_utf8 &str, uint16_t *numOfUTF8Chars) {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    const font_t *pf = resource_font(font);
    const int8_t char_w = pf->w;
    const int8_t char_h = pf->h;
    *numOfUTF8Chars = 0;
    unichar c = 0;
    StringReaderUtf8 reader(str);
    while ((c = reader.getUtf8Char()) != 0) {
        ++(*numOfUTF8Chars);
        if (c == '\n') {
            if (x + char_w > w) {
                w = x + char_w;
            }
            y += char_h;
            x = 0;
        } else {
            x += char_w;
        }
        h = y + char_h;
    }
    return { uint16_t(std::max(x, w)), uint16_t(h) };
}

void render_rect(Rect16 rc, Color clr) {
    display::fill_rect(rc, clr);
}

void render_rounded_rect(Rect16 rc, Color bg_clr, Color fg_clr, uint8_t rad, uint8_t flag) {
    display::draw_rounded_rect(rc, bg_clr, fg_clr, rad, flag);
}
