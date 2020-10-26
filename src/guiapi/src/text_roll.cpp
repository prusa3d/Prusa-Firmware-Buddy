#include "text_roll.hpp"
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

size_t txtroll_t::instance_counter = 0;

//invalidate at phase change
invalidate_t txtroll_t::Tick() {
    invalidate_t ret = invalidate_t::no;
    switch (phase) {
    case phase_t::uninitialized:
    case phase_t::idle:
    case phase_t::paused:
        break;
    case phase_t::init_roll:
        px_cd = 0;
        count = count_from_init;
        phase = count_from_init == 0 ? phase_t::idle : phase_t::wait_before_roll;
        ret = invalidate_t::yes;
        phase_progress = (wait_before_roll_ms + base_tick_ms - 1) / base_tick_ms;
        break;
    case phase_t::wait_before_roll:
        if ((--phase_progress) == 0) {
            phase = phase_t::go;
            ret = invalidate_t::yes;
            draw_progress = 0;
        }
        break;
    case phase_t::go:
        if (count > 0 || px_cd > 0) {
            if (px_cd == 0) {
                px_cd = font_w;
                count--;
                draw_progress++;
            }
            px_cd--;
        } else {
            phase = phase_t::wait_after_roll;
            phase_progress = (wait_after_roll_ms + base_tick_ms - 1) / base_tick_ms;
        }
        ret = invalidate_t::yes;
        break;
    case phase_t::wait_after_roll:
        if ((--phase_progress) == 0) {
            phase = phase_t::init_roll;
            ret = invalidate_t::yes;
        }
        break;
    }

    return ret;
}

void txtroll_t::Init(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment) {
    rect = rect_meas(rc, text, font, padding, alignment);
    count_from_init = meas(rect, text, font);
    font_w = font->w;
    phase = phase_t::init_roll;
}

void txtroll_t::RenderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font,
    color_t clr_back, color_t clr_text, padding_ui8_t padding, uint8_t alignment) const {
    switch (phase) {
    case phase_t::uninitialized:
    case phase_t::idle:
    case phase_t::init_roll:
    case phase_t::wait_before_roll:
        render_text_align(rc, text, font, clr_back, clr_text, padding, alignment); // normal render
        break;
    default:
        renderTextAlign(rc, text, font, clr_back, clr_text, padding, alignment); // rolling render
        break;
    }
}

void txtroll_t::renderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font,
    color_t clr_back, color_t clr_text, padding_ui8_t padding, uint8_t alignment) const {

    if (text.isNULLSTR()) {
        display::FillRect(rc, clr_back);
        return;
    }

    uint8_t unused_pxls = rect.Width() % font->w;
    if (unused_pxls) {
        Rect16 rc_unused_pxls = { int16_t(rect.Left() + rect.Width() - unused_pxls), rect.Top(), unused_pxls, rect.Height() };
        display::FillRect(rc_unused_pxls, clr_back);
    }

    //@@TODO make rolling native ability of render text - solves also character clipping
    //    const char *str = text;
    //    str += progress;
    // for now - just move to the desired starting character
    text.rewind();
    for (size_t i = 0; i < draw_progress; ++i) {
        text.getUtf8Char();
    }

    Rect16 set_txt_rc = rect;
    if (px_cd != 0) {
        set_txt_rc += Rect16::Left_t(px_cd);
        set_txt_rc -= Rect16::Width_t(px_cd);
    }

    if (!set_txt_rc.IsEmpty()) {
        fill_between_rectangles(&rc, &set_txt_rc, clr_back);
        render_text(set_txt_rc, /*str*/ text, font, clr_back, clr_text, 0);
    } else {
        display::FillRect(rc, clr_back);
    }
}

Rect16 txtroll_t::rect_meas(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint16_t flags) {

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

uint16_t txtroll_t::meas(Rect16 rc, string_view_utf8 text, const font_t *pf) {

    uint16_t meas_x = 0, len = text.computeNumUtf8CharsAndRewind();
    if (len * pf->w > rc.Width())
        meas_x = len - rc.Width() / pf->w;
    return meas_x;
}
