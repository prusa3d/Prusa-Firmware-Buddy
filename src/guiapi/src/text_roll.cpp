#include "text_roll.hpp"
#include <algorithm>

#include "display_helper.h"
#include "display.hpp"
#include "window.hpp"
#include "gui.hpp"
#include "../lang/string_view_utf8.hpp"
#include "ScreenHandler.hpp"
#include <common/conserve_cpu.hpp>

size_t txtroll_t::instance_counter = 0;

// invalidate at phase change
invalidate_t txtroll_t::Tick() {
    if (buddy::conserve_cpu().is_requested()) {
        // We are being asked to limit CPU, so we won't do rolling for a while.
        switch (phase) {
        case phase_t::uninitialized:
        case phase_t::idle:
        case phase_t::paused:
        case phase_t::wait_before_roll:
            return invalidate_t::no;
        default:
            phase = phase_t::init_roll;
            break;
        }
    }

    invalidate_t ret = invalidate_t::no;
    switch (phase) {
    case phase_t::uninitialized:
    case phase_t::idle:
    case phase_t::paused:
        break;
    case phase_t::init_roll:
        draw_progress = 0;
        px_cd = 0;
        count = hidden_char_cnt;
        phase = hidden_char_cnt == 0 ? phase_t::idle : phase_t::wait_before_roll;
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
            px_cd = 0;
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

void txtroll_t::Init(const Rect16 &rect, const string_view_utf8 &text, Font font, padding_ui8_t padding) {
    hidden_char_cnt = meas(rect, text, font, padding);
    font_w = resource_font(font)->w;
    phase = phase_t::init_roll;
}

void txtroll_t::render_text(const Rect16 &rect, const string_view_utf8 &text, Font font, Color clr_back, Color clr_text, padding_ui8_t padding, Align_t alignment) const {
    // render part of the 1st character
    font_t *font_pointer = resource_font(font);
    StringReaderUtf8 reader(text);
    if (px_cd != 0 && draw_progress > 0) {
        reader.skip(draw_progress - 1);
        unichar c = reader.getUtf8Char();
        point_ui16_t char_pos { (uint16_t)rect.Left() + padding.left, (uint16_t)rect.Top() + padding.top };
        render_char_part(char_pos, c, font_pointer, clr_back, clr_text, font_w - px_cd, font_w - 1);
    } else {
        reader.skip(draw_progress);
    }

    // render rest of the line
    Rect16 text_rect = Rect16(rect.Left() + padding.left + px_cd, rect.Top() + padding.top, rect.Width() - px_cd - padding.right, rect.Height() - padding.top - padding.bottom);
    render_text_align(text_rect, reader, font, clr_back, clr_text, padding_ui8_t(), text_flags(alignment, is_multiline::no, check_overflow::no), true);
}

uint16_t txtroll_t::meas(Rect16 rc, const string_view_utf8 &text, Font font, padding_ui8_t padding) {
    rc.CutPadding(padding);
    const auto font_w = resource_font(font)->w;
    uint16_t meas_x = 0, len = text.computeNumUtf8Chars();
    if (len * font_w > rc.Width()) {
        meas_x = len - rc.Width() / font_w;
    }
    return meas_x;
}
