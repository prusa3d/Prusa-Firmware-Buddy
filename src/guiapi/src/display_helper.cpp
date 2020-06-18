//display_helper.c

#include "display_helper.h"
#include "display.h"
#include "gui_timer.h"
#include "window.h"
#include "gui.h"

/// Fills space between two rectangles with a color
/// @r_in must be completely in @r_out, no check is done
/// TODO move to display
void fill_between_rectangles(const rect_ui16_t *r_out, const rect_ui16_t *r_in, color_t color) {
    // FIXME add check r_in in r_out; use some guitypes.c function

    const rect_ui16_t rc_t = { r_out->x, r_out->y, r_out->w, uint16_t(r_in->y - r_out->y) };
    display::FillRect(rc_t, color);

    const rect_ui16_t rc_b = { r_out->x, uint16_t(r_in->y + r_in->h), r_out->w, uint16_t((r_out->y + r_out->h) - (r_in->y + r_in->h)) };
    display::FillRect(rc_b, color);

    const rect_ui16_t rc_l = { r_out->x, r_in->y, uint16_t(r_in->x - r_out->x), r_in->h };
    display::FillRect(rc_l, color);

    const rect_ui16_t rc_r = { uint16_t(r_in->x + r_in->w), r_in->y, uint16_t((r_out->x + r_out->w) - (r_in->x + r_in->w)), r_in->h };
    display::FillRect(rc_r, color);
}

void render_text_align(rect_ui16_t rc, const char *text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags) {
    rect_ui16_t rc_pad = rect_ui16_sub_padding_ui8(rc, padding);
    if (flags & RENDER_FLG_WORDB) {
        //TODO: other alignments, following impl. is for LEFT-TOP
        uint16_t x;
        uint16_t y = rc_pad.y;
        int n;
        int i;
        const char *str = text;
        while ((n = font_line_chars(font, str, rc_pad.w)) && ((y + font->h) <= (rc_pad.y + rc_pad.h))) {
            x = rc_pad.x;
            i = 0;
            while (str[i] == ' ' || str[i] == '\n')
                i++;
            for (; i < n; i++) {
                display::DrawChar(point_ui16(x, y), str[i], font, clr0, clr1);
                x += font->w;
            }
            display::FillRect(rect_ui16(x, y, (rc_pad.x + rc_pad.w - x), font->h), clr0);
            str += n;
            y += font->h;
        }
        display::FillRect(rect_ui16(rc_pad.x, y, rc_pad.w, (rc_pad.y + rc_pad.h - y)), clr0);
        fill_between_rectangles(&rc, &rc_pad, clr0);
    } else {
        point_ui16_t wh_txt = font_meas_text(font, text);
        if (wh_txt.x && wh_txt.y) {
            rect_ui16_t rc_txt = rect_align_ui16(rc_pad, rect_ui16(0, 0, wh_txt.x, wh_txt.y), flags & ALIGN_MASK);
            rc_txt = rect_intersect_ui16(rc_pad, rc_txt);
            uint8_t unused_pxls = 0;
            if (strlen(text) * font->w > rc_txt.w) {
                unused_pxls = rc_txt.w % font->w;
            }

            const rect_ui16_t rect_in = { rc_txt.x, rc_txt.y, uint16_t(rc_txt.w - unused_pxls), rc_txt.h };
            fill_between_rectangles(&rc, &rect_in, clr0);
            display::DrawText(rc_txt, text, font, clr0, clr1);
        } else
            display::FillRect(rc, clr0);
    }
}

void render_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags) {
    color_t opt_clr;
    switch ((flags >> 8) & (ROPFN_SWAPBW | ROPFN_DISABLE)) {
    case ROPFN_SWAPBW | ROPFN_DISABLE:
        opt_clr = gui_defaults.color_disabled;
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
        rect_ui16_t rc_ico = rect_align_ui16(rc, rect_ui16(0, 0, wh_ico.x, wh_ico.y), flags & ALIGN_MASK);
        rc_ico = rect_intersect_ui16(rc, rc_ico);
        fill_between_rectangles(&rc, &rc_ico, opt_clr);
        display::DrawIcon(point_ui16(rc_ico.x, rc_ico.y), id_res, clr0, (flags >> 8) & 0x0f);
    } else
        display::FillRect(rc, opt_clr);
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

void roll_init(rect_ui16_t rc, const char *text, const font_t *font,
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

void render_roll_text_align(rect_ui16_t rc, const char *text, const font_t *font,
    padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text, const txtroll_t *roll) {
    if (roll->setup == TXTROLL_SETUP_INIT)
        return;

    if (text == 0) {
        display::FillRect(rc, clr_back);
        return;
    }

    uint8_t unused_pxls = roll->rect.w % font->w;
    if (unused_pxls) {
        rect_ui16_t rc_unused_pxls = { uint16_t(roll->rect.x + roll->rect.w - unused_pxls), roll->rect.y, unused_pxls, roll->rect.h };
        display::FillRect(rc_unused_pxls, clr_back);
    }

    const char *str = text;
    str += roll->progress;

    rect_ui16_t set_txt_rc = roll->rect;
    if (roll->px_cd != 0) {
        set_txt_rc.x += roll->px_cd;
        set_txt_rc.w -= roll->px_cd;
    }

    if (set_txt_rc.w && set_txt_rc.h) {
        fill_between_rectangles(&rc, &set_txt_rc, clr_back);
        display::DrawText(set_txt_rc, str, font, clr_back, clr_text);
    } else {
        display::FillRect(rc, clr_back);
    }
}
