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

void txtroll_t::Init(Rect16 rc, string_view_utf8 text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment) {
    rect = rect_meas(rc, text, font, padding, alignment);
    count = meas(rect, text, font);
    progress = px_cd = phase = 0;
    if (count == 0) {
        setup = TXTROLL_SETUP_IDLE;
    } else {
        setup = TXTROLL_SETUP_DONE;
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
