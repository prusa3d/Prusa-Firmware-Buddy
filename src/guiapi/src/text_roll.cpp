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

invalidate_t txtroll_t::Tick() {
    invalidate_t ret = invalidate_t::no;
    switch (phase) {
    case phase_t::init:
        //gui_timer_change_txtroll_peri_delay(TEXT_ROLL_DELAY_MS, pWin);
        ret = invalidate_t::yes;
        break;
    case phase_t::setup_done:
        //gui_timer_change_txtroll_peri_delay(TEXT_ROLL_DELAY_MS, pWin);
        phase = phase_t::go;
        ret = invalidate_t::yes;
        break;

    case phase_t::go:
        if (count > 0 || px_cd > 0) {
            if (px_cd == 0) {
                px_cd = font_w;
                count--;
                progress++;
            }
            px_cd--;
            ret = invalidate_t::yes;
        } else {
            phase = phase_t::stop;
        }
        break;
    case phase_t::stop:
        phase = phase_t::restart;
        //gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, pWin);
        break;
    case phase_t::restart:
        phase = phase_t::init;
        ret = invalidate_t::yes;
        break;
    case phase_t::idle:
        break;
    }

    return ret;
}

void txtroll_t::Init(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment) {
    rect = rect_meas(rc, text, font, padding, alignment);
    count = meas(rect, text, font);
    progress = px_cd = 0;
    font_w = font->w;
    if (count == 0) {
        phase = phase_t::idle;
    } else {
        phase = phase_t::setup_done;
    }
}

void txtroll_t::RenderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text) const {
    if (phase == phase_t::init)
        return;

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
    for (size_t i = 0; i < progress; ++i) {
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

void txtroll_t::Reset(window_t *pWin) {
    count = px_cd = progress = 0;
    phase = phase_t::init;

    gui_timer_create_txtroll(pWin, TEXT_ROLL_INITIAL_DELAY_MS);
    //    gui_timer_restart_txtroll(this);
    //gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, this);
}
