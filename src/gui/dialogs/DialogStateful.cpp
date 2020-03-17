#include "DialogStateful.hpp"
#include "DialogRadioButton.hpp"
#include "gui.h"

#include "display_helper.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "button_draw.h"
#include "window_dlg_change.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_msgbox.h"

extern window_t *window_1; //current popup window, C-code remain

const char *const test_title = "TEST";

//*****************************************************************************
//DlgVars
DlgVars::DlgVars()
    : phase(0)
    , prev_phase(-1)
    , progress(0)
    , prev_progress(-1) {}

//*****************************************************************************
//DlgStatemachine
/*
DlgStatemachine::DlgStatemachine(const char *tit)
    :  color_back(gui_defaults.color_back)
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , font_title(gui_defaults.font_big)
    , padding(gui_defaults.padding)
    , flags(0)
    , last_text_h(0)
    , title(tit) {
}*/

//*****************************************************************************

IDialogStateful::IDialogStateful(const char *name, int16_t WINDOW_CLS_)
    : IDialog(winCreate(WINDOW_CLS))
    , WINDOW_CLS(WINDOW_CLS_)
    , id_capture(window_capture())
#warning check id_capture(window_capture())
    , color_back(gui_defaults.color_back)
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , font_title(gui_defaults.font_big)
    , padding(gui_defaults.padding)
    , flags(0)
    , last_text_h(0)
    , title(name) {
    //  err dlg nezna stavy a count
    //, id(window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg))

    window_1 = this; //todo
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
}

bool IDialogStateful::Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (!can_change(phase))
        return false;
    dlg_vars.phase = phase;
    flags |= DLG_PHA_CH;
#warning do progress here
    //dlg.vars.phase = phase;
    //dlg.flags |= DLG_PHA_CH;
    gui_invalidate();
    return true;
}

IDialogStateful::~IDialogStateful() {
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
}

extern window_t *window_1; //current popup window
/*
void window_dlg_statemachine_init(window_dlg_statemachine_t *window) {
    if (rect_empty_ui16(window->win.rect)) //use display rect if current rect is empty
        window->win.rect = rect_ui16(0, 0, display->w, display->h);
    window->win.flg |= WINDOW_FLG_ENABLED; //enabled by default
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;
    window->vars.phase = 0;
    window->vars.prev_phase = -1;
    window->vars.progress = 0;
    window->flags = 0;
}
*/
void IDialogStateful::draw_frame() {
    rect_ui16_t rc = rect;
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
}

//this should be moved elswhere
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
//this should be moved elswhere
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
    if (dlg_vars.progress <= 100) {
        if (flags & DLG_PRO_CH)
            progress_draw(rect, font_title, color_back, color_text, padding, dlg_vars.progress);
    } else {
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

/*
void IDialogStateful::_draw_phase_text(IDialogStateful *window) {
    rect_ui16_t rc_sta = window->rect;
    size_t nl; //number of new lines
    const char *s = window->_ths->p_states[window->vars.phase].text;
    for (nl = 0; s[nl]; s[nl] == '\n' ? nl++ : *s++)
        ; //count '\n' in s
    rc_sta.h = 30 + window->font_title->h * nl;
    rc_sta.y += (30 + 46);
    rc_sta.x += 2;
    rc_sta.w -= 4;

    //erase remains of previous text if it was longer
    //prerelease hack todo text window just should be CENTER_TOP aligned and bigger
    int h_diff = window->last_text_h - rc_sta.h;
    if (h_diff > 0) {
        rect_ui16_t rc = rc_sta;
        rc.h = window->last_text_h - rc_sta.h;
        rc.y += rc_sta.h;
        display->fill_rect(rc, window->color_back);
    }

    window->last_text_h = rc_sta.h;

    render_text_align(rc_sta, window->_ths->p_states[window->vars.phase].text, window->font_title,
        window->color_back, window->color_text, window->padding, ALIGN_CENTER);
}

void IDialogStateful::draw(IDialogStateful *window) {
    if ((window->f_visible) && ((size_t)(window->dlg_vars.phase) < window->_ths->count)) {
        rect_ui16_t rc = window->rect;

        if (window->f_invalid) {
            display->fill_rect(rc, window->color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.h = 30; // 30pixels for title
            // TODO: - icon
            //			rc_tit.w -= 30;
            //			rc_tit.x += 30;
            //title
            render_text_align(rc_tit, window->title, window->font_title,
                window->color_back, window->color_text, window->padding, ALIGN_CENTER);

            window->f_invalid = 0;
            window->flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PPR_CH;
        }
        //DLG_PHA_CH == DLG_TXT_CH
        if (window->flags & DLG_TXT_CH) //text changed
        {
            _window_dlg_statemachine_draw_phase_text(window);
            window->flags &= ~DLG_TXT_CH;
        }
        //button knows when it needs to be repainted
        window->_ths->p_states[window->vars.phase].radio_btn.Draw();

        if (window->flags & DLG_PRX_CH) //any progress changed
        {
            window->_ths->p_states[window->vars.phase].progress_draw(window);
            window->flags &= ~DLG_PRX_CH;
        }
        if (window->flags & DLG_DRA_FR) { //draw frame
            _window_dlg_statemachine_draw_frame(window);
            window->flags &= ~DLG_DRA_FR;
        }
    }
}

void IDialogStateful::event(IDialogStateful *window, uint8_t event, void *param) {
    RadioButton &btn = window->_ths->p_states[window->vars.phase].radio_btn;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    //case WINDOW_EVENT_BTN_UP:
    case WINDOW_EVENT_CLICK:
        btn.Click();
        return;
    case WINDOW_EVENT_ENC_UP:
        ++btn;
        return;
    case WINDOW_EVENT_ENC_DN:
        --btn;
        return;
    }
}*/
