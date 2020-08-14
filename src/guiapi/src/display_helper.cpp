//display_helper.c

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

//#define UNACCENT

#ifdef UNACCENT
std::pair<const char *, uint8_t> ConvertUnicharToFontCharIndex(unichar c) {
    // for now we have a translation table and in the future we'll have letters with diacritics too (i.e. more font bitmaps)
    const auto &a = UnaccentTable::Utf8RemoveAccents(c);
    return std::make_pair(a.str, a.size); // we are returning some number of characters to replace the input utf8 character
}
#endif

/// Draws a text into the specified rectangle @rc
/// If a character does not fit into the rectangle the drawing is stopped
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// \returns true if whole text was written
/// Extracted from st7789v implementation, where it shouldn't be @@TODO cleanup
size_ui16_t render_text(Rect16 rc, string_view_utf8 str, const font_t *pf, color_t clr_bg, color_t clr_fg, uint16_t flags) {
    int x = rc.Left();
    int y = rc.Top();

    const uint16_t w = pf->w; //char width
    const uint16_t h = pf->h; //char height
    // prepare for stream processing
    unichar c = 0;
    text_wrapper<ram_buffer, const font_t *> wrapper(rc.Width(), pf);
    no_wrap text_plain;

    while (true) {
        c = (flags & RENDER_FLG_WORDB)
            ? wrapper.character(str)
            : text_plain.character(str);

        if (c == 0) {
            break;
        }
        if (c == '\n' && (flags & RENDER_FLG_WORDB)) {
            y += h;
            x = rc.Left();
            continue;
        }
#ifdef UNACCENT
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
    return size_ui16_t { rc.Width(), static_cast<std::uint16_t>(y - rc.Top()) };
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

void render_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags) {
    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);
    if (flags & RENDER_FLG_WORDB) {
        size_ui16_t s = render_text(rc_pad, text, font, clr0, clr1, RENDER_FLG_WORDB);
        // hack for broken text wrapper ... and for too long texts as well
        if ((rc_pad.Top() + s.h) < (rc_pad.Top() + rc_pad.Height())) {
            display::FillRect(Rect16(rc_pad.Left(), rc_pad.Top() + s.h, rc_pad.Width(), rc_pad.Height() - s.h), clr0);
            fill_between_rectangles(&rc, &rc_pad, clr0);
        }

        return;
    }

    // 1st pass reading the string_view_utf8 - font_meas_text also computes the number of utf8 characters (i.e. individual bitmaps) in the input string
    uint16_t strlen_text = 0;
    point_ui16_t wh_txt = font_meas_text(font, &text, &strlen_text);
    if (!wh_txt.x || !wh_txt.y) {
        display::FillRect(rc, clr0);
        return;
    }

    Rect16 rc_txt = Rect16(0, 0, wh_txt.x, wh_txt.y);
    rc_txt.Align(rc_pad, flags & ALIGN_MASK);
    rc_txt = rc_txt.Intersection(rc_pad);
    const uint8_t unused_pxls = (strlen_text * font->w <= rc_txt.Width()) ? 0 : rc_txt.Width() % font->w;

    const Rect16 rect_in = rc_txt - Rect16::Width_t(unused_pxls);
    fill_between_rectangles(&rc, &rect_in, clr0);
    text.rewind();
    // 2nd pass reading the string_view_utf8 - draw the text
    render_text(rc_txt, text, font, clr0, clr1, 0);
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

void roll_text_phasing(window_t *pWin, font_t *font, txtroll_t *roll) {
    if (roll->setup == TXTROLL_SETUP_IDLE)
        return;
    switch (roll->phase) {
    case ROLL_SETUP:
        gui_timer_change_txtroll_peri_delay(TEXT_ROLL_DELAY_MS, pWin);
        if (roll->setup == TXTROLL_SETUP_DONE)
            roll->phase = ROLL_GO;
        pWin->Invalidate();
        break;
    case ROLL_GO:
        if (roll->count > 0 || roll->px_cd > 0) {
            if (roll->px_cd == 0) {
                roll->px_cd = font->w;
                roll->count--;
                roll->progress++;
            }
            roll->px_cd--;
            pWin->Invalidate();
        } else {
            roll->phase = ROLL_STOP;
        }
        break;
    case ROLL_STOP:
        roll->phase = ROLL_RESTART;
        gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, pWin);
        break;
    case ROLL_RESTART:
        roll->setup = TXTROLL_SETUP_INIT;
        roll->phase = ROLL_SETUP;
        pWin->Invalidate();
        break;
    }
}

void roll_init(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment, txtroll_t *roll) {
    roll->rect = roll_text_rect_meas(rc, text, font, padding, alignment);
    roll->count = text_rolls_meas(roll->rect, text, font);
    roll->progress = roll->px_cd = roll->phase = 0;
    if (roll->count == 0) {
        roll->setup = TXTROLL_SETUP_IDLE;
    } else {
        roll->setup = TXTROLL_SETUP_DONE;
    }
}

void render_roll_text_align(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text, const txtroll_t *roll) {
    if (roll->setup == TXTROLL_SETUP_INIT)
        return;

    if (text.isNULLSTR()) {
        display::FillRect(rc, clr_back);
        return;
    }

    uint8_t unused_pxls = roll->rect.Width() % font->w;
    if (unused_pxls) {
        Rect16 rc_unused_pxls = { int16_t(roll->rect.Left() + roll->rect.Width() - unused_pxls), roll->rect.Top(), unused_pxls, roll->rect.Height() };
        display::FillRect(rc_unused_pxls, clr_back);
    }

    //@@TODO make rolling native ability of render text - solves also character clipping
    //    const char *str = text;
    //    str += roll->progress;
    // for now - just move to the desired starting character
    text.rewind();
    for (size_t i = 0; i < roll->progress; ++i) {
        text.getUtf8Char();
    }

    Rect16 set_txt_rc = roll->rect;
    if (roll->px_cd != 0) {
        set_txt_rc += Rect16::Left_t(roll->px_cd);
        set_txt_rc -= Rect16::Width_t(roll->px_cd);
    }

    if (!set_txt_rc.IsEmpty()) {
        fill_between_rectangles(&rc, &set_txt_rc, clr_back);
        render_text(set_txt_rc, /*str*/ text, font, clr_back, clr_text, 0);
    } else {
        display::FillRect(rc, clr_back);
    }
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
    return point_ui16((uint16_t)std::max(x, w), (uint16_t)h);
}

uint16_t text_rolls_meas(Rect16 rc, string_view_utf8 text, const font_t *pf) {

    uint16_t meas_x = 0, len = text.computeNumUtf8CharsAndRewind();
    if (len * pf->w > rc.Width())
        meas_x = len - rc.Width() / pf->w;
    return meas_x;
}

Rect16 roll_text_rect_meas(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint16_t flags) {

    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);
    uint16_t numOfUTF8Chars;
    point_ui16_t wh_txt = font_meas_text(font, &text, &numOfUTF8Chars);
    Rect16 rc_txt = { 0, 0, 0, 0 };
    if (wh_txt.x && wh_txt.y) {
        rc_txt = Rect16(0, 0, wh_txt.x, wh_txt.y);
        rc_txt.Align(rc_pad, flags & ALIGN_MASK);
        rc_txt = rc_txt.Intersection(rc_pad);
    }
    return rc_txt;
}
