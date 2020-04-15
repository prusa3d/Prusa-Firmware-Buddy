#include "DialogStateful.hpp"
#include "DialogRadioButton.hpp"
#include "gui.h"

//*****************************************************************************
IDialogStateful::IDialogStateful(const char *name, int16_t WINDOW_CLS_)
    : IDialog(WINDOW_CLS_)
    , id_capture(window_capture())
    , color_back(gui_defaults.color_back)
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , font_title(gui_defaults.font_big)
    , padding(gui_defaults.padding)
    , flags(0)
    , last_text_h(0)
    , phase(0)
    , progress(-1)
    , title(name) {
    window_popup_ptr = this;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
}

bool IDialogStateful::Change(uint8_t phs, uint8_t progress_tot, uint8_t progr) {
    if (!can_change(phs))
        return false;
    if (phase != phs) {
        phase = phs;
        flags |= DLG_PHA_CH;
    }
    if (progress_tot != progress) {
        progress = progress_tot;
        flags |= DLG_PRO_CH;
    }
    gui_invalidate();
    return true;
}

IDialogStateful::~IDialogStateful() {
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
}

void IDialogStateful::draw_frame() {
    rect_ui16_t rc = rect;
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
}

//todo this should be moved elswhere
void progress_draw(rect_ui16_t win_rect, font_t *font, color_t color_back,
    color_t color_text, padding_ui8_t padding, uint8_t progress) {
    rect_ui16_t rc_pro = win_rect; //must copy it
    char text[16];
    rc_pro.x += 10;
    rc_pro.w -= 20;
    rc_pro.h = 16;
    rc_pro.y += 30;
    uint16_t w = rc_pro.w;
    rc_pro.w = w * progress / 100;
    display->fill_rect(rc_pro, COLOR_ORANGE);
    rc_pro.x += rc_pro.w;
    rc_pro.w = w - rc_pro.w;
    display->fill_rect(rc_pro, COLOR_GRAY);
    rc_pro.y += rc_pro.h;
    rc_pro.w = win_rect.w - 120;
    rc_pro.x = win_rect.x + 60;
    rc_pro.h = 30;
    sprintf(text, "%d%%", progress);
    render_text_align(rc_pro, text, font, color_back, color_text, padding, ALIGN_CENTER);
}

//todo this should be moved elswhere
void progress_clr(rect_ui16_t win_rect, font_t *font, color_t color_back) {
    rect_ui16_t rc_pro = win_rect; //must copy it
    rc_pro.x += 10;
    rc_pro.w -= 20;
    rc_pro.h = 16;
    rc_pro.y += 30;
    display->fill_rect(rc_pro, color_back);
    rc_pro.y += rc_pro.h;
    rc_pro.w = win_rect.w - 120;
    rc_pro.x = win_rect.x + 60;
    rc_pro.h = 30;
    display->fill_rect(rc_pro, color_back);
}

void IDialogStateful::draw_progress() {
    if (progress <= 100) {
        if (flags & DLG_PRO_CH)
            progress_draw(rect, font_title, color_back, color_text, padding, progress);
    } else {
        //do not draw progress at all
        progress_clr(rect, font_title, color_back);
    }
}

void IDialogStateful::draw_phase_text(const char *text) {
    rect_ui16_t rc_sta = rect;
    size_t nl; //number of new lines
    const char *s = text;
    for (nl = 0; s[nl]; s[nl] == '\n' ? nl++ : *s++)
        ; //count '\n' in s
    rc_sta.h = 30 + font_title->h * nl;
    rc_sta.y += (30 + 46);
    rc_sta.x += 2;
    rc_sta.w -= 4;

    //erase remains of previous text if it was longer
    //prerelease hack todo text window just should be CENTER_TOP aligned and bigger
    int h_diff = last_text_h - rc_sta.h;
    if (h_diff > 0) {
        rect_ui16_t rc = rc_sta;
        rc.h = last_text_h - rc_sta.h;
        rc.y += rc_sta.h;
        display->fill_rect(rc, color_back);
    }

    last_text_h = rc_sta.h;

    render_text_align(rc_sta, text, font_title,
        color_back, color_text, padding, ALIGN_CENTER);
}
