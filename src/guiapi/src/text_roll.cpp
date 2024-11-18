#include "text_roll.hpp"
#include <algorithm>

#include "display_helper.h"
#include "display.hpp"
#include "window.hpp"
#include "gui.hpp"
#include "../lang/string_view_utf8.hpp"
#include "../common/str_utils.hpp"
#include "ScreenHandler.hpp"

size_t txtroll_t::instance_counter = 0;

// invalidate at phase change
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

static uint8_t runtime_width(Font font) {
    return resource_font(font)->w;
}

void txtroll_t::Init(Rect16 rc, const string_view_utf8 &text, Font font,
    padding_ui8_t padding, Align_t alignment) {
    rect = rect_meas(rc, text, font, padding, alignment);
    count_from_init = meas(rect, text, font);
    font_w = runtime_width(font);
    phase = phase_t::init_roll;
}

void txtroll_t::render_text(Rect16 rc, const string_view_utf8 &text, Font font,
    Color clr_back, Color clr_text, padding_ui8_t padding, Align_t alignment, bool fill_rect) const {
    switch (phase) {
    case phase_t::uninitialized:
    case phase_t::idle:
    case phase_t::init_roll:
    case phase_t::wait_before_roll:
        render_text_align(rc, text, font, clr_back, clr_text, padding, alignment, fill_rect); // normal render
        break;
    default:
        render_rolling_text_align(rc, text, font, clr_back, clr_text, padding, alignment, fill_rect); // rolling render
        break;
    }
}

void txtroll_t::render_rolling_text_align(Rect16 rc, const string_view_utf8 &text, Font font,
    Color clr_back, Color clr_text, [[maybe_unused]] padding_ui8_t padding, [[maybe_unused]] Align_t alignment, bool fill_rect) const {

    if (text.isNULLSTR()) {
        if (fill_rect) {
            display::fill_rect(rc, clr_back);
        }
        return;
    }

    uint8_t unused_pxls = rect.Width() % runtime_width(font);
    if (unused_pxls) {
        Rect16 rc_unused_pxls = { int16_t(rect.Left() + rect.Width() - unused_pxls), rect.Top(), unused_pxls, rect.Height() };
        if (fill_rect) {
            display::fill_rect(rc_unused_pxls, clr_back);
        }
    }

    Rect16 set_txt_rc = rect;
    if (px_cd != 0) {
        set_txt_rc += Rect16::Left_t(px_cd);
        set_txt_rc -= Rect16::Width_t(px_cd);
    }

    if (!set_txt_rc.IsEmpty()) {
        Rect16 text_drawn_at(set_txt_rc.TopLeft(), render_text_singleline(set_txt_rc, StringReaderUtf8(text).skip(draw_progress), resource_font(font), clr_back, clr_text));
        if (fill_rect) {
            fill_between_rectangles(&rc, &text_drawn_at, clr_back);
        }
    } else {
        if (fill_rect) {
            display::fill_rect(rc, clr_back);
        }
    }
}

Rect16 txtroll_t::rect_meas(Rect16 rc, const string_view_utf8 &text, Font font, padding_ui8_t padding, Align_t alignment) {

    Rect16 rc_pad = rc;
    rc_pad.CutPadding(padding);
    uint16_t numOfUTF8Chars;
    size_ui16_t txt_size = font_meas_text(font, text, &numOfUTF8Chars);
    Rect16 rc_txt = { 0, 0, 0, 0 };
    if (txt_size.w && txt_size.h) {
        rc_txt = Rect16(0, 0, txt_size.w, txt_size.h);
        rc_txt.Align(rc_pad, alignment);
        rc_txt = rc_txt.Intersection(rc_pad);
    }
    return rc_txt;
}

uint16_t txtroll_t::meas(Rect16 rc, const string_view_utf8 &text, Font pf) {
    uint16_t meas_x = 0, len = text.computeNumUtf8Chars();
    if (len * runtime_width(pf) > rc.Width()) {
        meas_x = len - rc.Width() / runtime_width(pf);
    }
    return meas_x;
}
