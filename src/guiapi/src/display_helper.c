//display_helper.c

#include "display_helper.h"
#include "display.h"
#include "gui_timer.h"
#include "window.h"

void render_text_align(rect_ui16_t rc, const char *text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags) {
    rect_ui16_t rc_pad = rect_ui16_sub_padding_ui8(rc, padding);
    if (flags & RENDER_FLG_WORDB) {
        //TODO: other alignments, following impl. is for LEFT-TOP
        uint16_t x;
        uint16_t y = rc_pad.y;
        int n;
        const char *str = text;
        while ((n = font_line_chars(font, str, rc_pad.w)) && ((y + font->h) <= (rc_pad.y + rc_pad.h))) {
            x = rc_pad.x;
            int i = 0;
            while (str[i] == ' ' || str[i] == '\n')
                i++;
            for (; i < n; i++) {
                display->draw_char(point_ui16(x, y), str[i], font, clr0, clr1);
                x += font->w;
            }
            display->fill_rect(rect_ui16(x, y, (rc_pad.x + rc_pad.w - x), font->h), clr0);
            str += n;
            y += font->h;
        }
        display->fill_rect(rect_ui16(rc_pad.x, y, rc_pad.w, (rc_pad.y + rc_pad.h - y)), clr0);
        rect_ui16_t rc_t = { rc.x, rc.y, rc.w, rc_pad.y - rc.y };
        rect_ui16_t rc_b = { rc.x, rc_pad.y + rc_pad.h, rc.w, (rc.y + rc.h) - (rc_pad.y + rc_pad.h) };
        rect_ui16_t rc_l = { rc.x, rc.y, rc_pad.x - rc.x, rc.h };
        rect_ui16_t rc_r = { rc_pad.x + rc_pad.w, rc.y, (rc.x + rc.w) - (rc_pad.x + rc_pad.w), rc.h };
        display->fill_rect(rc_t, clr0);
        display->fill_rect(rc_b, clr0);
        display->fill_rect(rc_l, clr0);
        display->fill_rect(rc_r, clr0);
    } else {
        point_ui16_t wh_txt = font_meas_text(font, text);
        if (wh_txt.x && wh_txt.y) {
            rect_ui16_t rc_txt = rect_align_ui16(rc_pad, rect_ui16(0, 0, wh_txt.x, wh_txt.y), flags & ALIGN_MASK);
            rc_txt = rect_intersect_ui16(rc_pad, rc_txt);
            uint8_t unused_pxls = 0;
            if (strlen(text) * font->w > rc_txt.w) {
                unused_pxls = rc_txt.w % font->w;
            }
            rect_ui16_t rc_t = { rc.x, rc.y, rc.w, rc_txt.y - rc.y };
            rect_ui16_t rc_b = { rc.x, rc_txt.y + rc_txt.h, rc.w, (rc.y + rc.h) - (rc_txt.y + rc_txt.h) };
            rect_ui16_t rc_l = { rc.x, rc.y, rc_txt.x - rc.x, rc.h };
            rect_ui16_t rc_r = { rc_txt.x + rc_txt.w - unused_pxls, rc.y, (rc.x + rc.w) - (rc_txt.x + rc_txt.w) + unused_pxls, rc.h };
            display->fill_rect(rc_t, clr0);
            display->fill_rect(rc_b, clr0);
            display->fill_rect(rc_l, clr0);
            display->fill_rect(rc_r, clr0);
            display->draw_text(rc_txt, text, font, clr0, clr1);
        } else
            display->fill_rect(rc, clr0);
    }
}

void render_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags) {
    point_ui16_t wh_ico = icon_meas(resource_ptr(id_res));
    if (wh_ico.x && wh_ico.y) {
        rect_ui16_t rc_ico = rect_align_ui16(rc, rect_ui16(0, 0, wh_ico.x, wh_ico.y), flags & ALIGN_MASK);
        rc_ico = rect_intersect_ui16(rc, rc_ico);
        rect_ui16_t rc_t = { rc.x, rc.y, rc.w, rc_ico.y - rc.y };
        rect_ui16_t rc_b = { rc.x, rc_ico.y + rc_ico.h, rc.w, (rc.y + rc.h) - (rc_ico.y + rc_ico.h) };
        rect_ui16_t rc_l = { rc.x, rc.y, rc_ico.x - rc.x, rc.h };
        rect_ui16_t rc_r = { rc_ico.x + rc_ico.w, rc.y, (rc.x + rc.w) - (rc_ico.x + rc_ico.w), rc.h };
        display->fill_rect(rc_t, ((flags >> 8) & ROPFN_SWAPBW) ? clr0 ^ 0xffffffff : clr0);
        display->fill_rect(rc_b, ((flags >> 8) & ROPFN_SWAPBW) ? clr0 ^ 0xffffffff : clr0);
        display->fill_rect(rc_l, ((flags >> 8) & ROPFN_SWAPBW) ? clr0 ^ 0xffffffff : clr0);
        display->fill_rect(rc_r, ((flags >> 8) & ROPFN_SWAPBW) ? clr0 ^ 0xffffffff : clr0);
        display->draw_icon(point_ui16(rc_ico.x, rc_ico.y), id_res, clr0, (flags >> 8) & 0x0f);
    } else
        display->fill_rect(rc, clr0);
}

void roll_text_phasing(int16_t win_id, font_t *font, txtroll_t *roll) {
    if (roll->setup == TXTROLL_SETUP_IDLE)
        return;

    switch (roll->phase) {
    case ROLL_SETUP:
        gui_timer_change_txtroll_peri_delay(TEXT_ROLL_DELAY_MS, win_id);
        if (roll->setup == TXTROLL_SETUP_DONE)
            roll->phase = ROLL_GO;
        window_invalidate(win_id);
        break;
    case ROLL_GO:
        if (roll->count > 0 || roll->px_cd > 0) {
            if (roll->px_cd == 0) {
                roll->px_cd = font->w;
                roll->count--;
                roll->progress++;
            }
            roll->px_cd--;
            window_invalidate(win_id);
        } else {
            roll->phase = ROLL_STOP;
        }
        break;
    case ROLL_STOP:
        roll->phase = ROLL_RESTART;
        gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, win_id);
        break;
    case ROLL_RESTART:
        roll->setup = TXTROLL_SETUP_INIT;
        roll->phase = ROLL_SETUP;
        window_invalidate(win_id);
        break;
    }
}

void render_roll_text_align(rect_ui16_t rc, const char *text, font_t *font,
    padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text, txtroll_t *roll) {

    if (text == 0) {
        display->fill_rect(rc, clr_back);
        return;
    }

    if (roll->setup == TXTROLL_SETUP_INIT) {
        roll->rect = roll_text_rect_meas(rc, text, font, padding, alignment);
        roll->count = text_rolls_meas(roll->rect, text, font);
        roll->progress = roll->px_cd = roll->phase = 0;
        if (roll->count == 0) {
            roll->setup = TXTROLL_SETUP_IDLE;
        } else {
            uint8_t unused_pxls = roll->rect.w % font->w;
            if (unused_pxls) {
                rect_ui16_t rc_unused_pxls = { roll->rect.x + roll->rect.w - unused_pxls, roll->rect.y, unused_pxls, roll->rect.h };
                display->fill_rect(rc_unused_pxls, clr_back);
            }
            roll->setup = TXTROLL_SETUP_DONE;
        }
    }

    const char *str = text;
    str += roll->progress;

    rect_ui16_t set_txt_rc = roll->rect;
    if (roll->px_cd != 0) {
        set_txt_rc.x += roll->px_cd;
        set_txt_rc.w -= roll->px_cd;
    }

    if (set_txt_rc.w && set_txt_rc.h) {
        rect_ui16_t rc_t = { rc.x, rc.y, rc.w, set_txt_rc.y - rc.y };
        rect_ui16_t rc_b = { rc.x, set_txt_rc.y + set_txt_rc.h, rc.w, (rc.y + rc.h) - (set_txt_rc.y + set_txt_rc.h) };
        rect_ui16_t rc_l = { rc.x, rc.y, set_txt_rc.x - rc.x, rc.h };
        rect_ui16_t rc_r = { set_txt_rc.x + set_txt_rc.w, rc.y, (rc.x + rc.w) - (set_txt_rc.x + set_txt_rc.w), rc.h };
        display->fill_rect(rc_t, clr_back);
        display->fill_rect(rc_b, clr_back);
        display->fill_rect(rc_l, clr_back);
        display->fill_rect(rc_r, clr_back);

        display->draw_text(set_txt_rc, str, font, clr_back, clr_text);
    } else {
        display->fill_rect(rc, clr_back);
    }
}
