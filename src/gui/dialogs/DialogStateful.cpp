#include "DialogStateful.hpp"
#include "window_dlg_statemachine.h"
#include "gui.h"

//choose 1
//#define DBG_LOAD_PROGRESS 1
#define DBG_LOAD_PROGRESS 0

//#define DLG_FRAME_ENA 1
#define DLG_FRAME_ENA 0

//dialog flags bitshift
#define DLG_SHI_MOD 4  // mode shift
#define DLG_SHI_CHG 14 // change flag shift

#if DLG_FRAME_ENA == 1
    #define DLG_DRA_FR 0x0800 // draw frame
#else
    #define DLG_DRA_FR 0x0000                             // draw frame
#endif                                                    //DLG_FRAME_ENA == 1
#define DLG_BTN_CH 0x1000                                 // button changed
#define DLG_TXT_CH 0x2000                                 // text changed
#define DLG_PRO_CH 0x4000                                 // progress changed
#define DLG_PPR_CH 0x8000                                 // part progress changed
#define DLG_PRX_CH (DLG_PRO_CH | DLG_PPR_CH)              // some progress changed
#define DLG_PHA_CH (DLG_PRX_CH | DLG_BTN_CH | DLG_TXT_CH) // phase changed
//dialog flags bitmasks
#define DLG_MSK_MOD 0x0003     // mode mask
#define DLG_MSK_CHG DLG_PHA_CH // change flag mask

//data structure defining 1 stateful state

extern window_t *window_1; //current popup window, C-code remain

extern const _dlg_state test_states[14];

const char *const test_title = "TEST";
static _cl_dlg cl_dlg = { test_title, test_states, 14 }; //todo c remains

constexpr window_dlg_statemachine_t dlg_init() {
    window_dlg_statemachine_t ret = {};
    ret._ths = &cl_dlg;
    return ret;
}

static window_dlg_statemachine_t dlg = dlg_init(); //todo c remains

//*****************************************************************************

IDialogStateful::IDialogStateful(const char *name)
    : id_capture(window_capture())
    , _name(name)
    //  err dlg nezna stavy a count
    , id(window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg)) {

    window_1 = (window_t *)&dlg; //todo
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);
}

void IDialogStateful::Change(uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    dlg.vars.phase = phase;
    dlg.flags |= DLG_PHA_CH;
    gui_invalidate();
}

IDialogStateful::~IDialogStateful() {
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
}

#include "display_helper.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "button_draw.h"
#include "window_dlg_change.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_msgbox.h"

int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;

extern window_t *window_1; //current popup window

void set_repaint_btn_flg(window_dlg_statemachine_t *window) {
    window->flags |= DLG_BTN_CH;
}

int is_repaint_btn_flg(window_dlg_statemachine_t *window) {
    return window->flags | DLG_BTN_CH;
}

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
    //window->vars.prev_progress = 0;
    //window->vars.part_progress = 0;
    window->flags = 0;

    //window->vars.tick_part_start = HAL_GetTick();
    //window->vars.time_total = _phase_time_total(window->_ths->count - 1, window->_ths);
}

rect_ui16_t _get_dlg_statemachine_button_size(window_dlg_statemachine_t *window) {
    rect_ui16_t rc_btn = window->win.rect;
    rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
    rc_btn.h = 30;
    rc_btn.x += 6;
    rc_btn.w -= 12;
    return rc_btn;
}

void window_dlg_statemachine_draw_1bt(window_dlg_statemachine_t *window) {
    rect_ui16_t rc_btn = _get_dlg_statemachine_button_size(window);
    const char *label = window->_ths->p_states[window->vars.phase].p_button->labels[0];
    int enabled = window->_ths->p_states[window->vars.phase].p_button->flags & BT_ENABLED ? 1 : 0;
    button_draw(rc_btn, label, window->font_title, enabled);
}

void window_dlg_statemachine_draw_0bt(window_dlg_statemachine_t *window) {
    rect_ui16_t rc_btn = _get_dlg_statemachine_button_size(window);

    display->fill_rect(rc_btn, window->color_back);
}

void _window_dlg_statemachine_draw_frame(window_dlg_statemachine_t *window) {
    rect_ui16_t rc = window->win.rect;
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
    display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
}

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

void window_dlg_statemachine_draw_progress_tot(window_dlg_statemachine_t *window) {
    if (window->flags & DLG_PRO_CH)
        progress_draw(window->win.rect, window->font_title, window->color_back,
            window->color_text, window->padding, window->vars.progress);
}
/*
void window_dlg_statemachine_draw_progress_part(window_dlg_statemachine_t *window) {
    if (window->flags & DLG_PPR_CH)
        progress_draw(window->win.rect, window->font_title, window->color_back,
            window->color_text, window->padding, window->vars.part_progress);
}*/

void window_dlg_statemachine_draw_progress_none(window_dlg_statemachine_t *window) {
    progress_clr(window->win.rect, window->font_title, window->color_back);
}

void _window_dlg_statemachine_draw_phase_text(window_dlg_statemachine_t *window) {
    rect_ui16_t rc_sta = window->win.rect;
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

void window_dlg_statemachine_draw(window_dlg_statemachine_t *window) {
    if ((window->win.f_visible) && ((size_t)(window->vars.phase) < window->_ths->count)) {
        rect_ui16_t rc = window->win.rect;

        if (window->win.f_invalid) {
            display->fill_rect(rc, window->color_back);
            rect_ui16_t rc_tit = rc;
            rc_tit.h = 30; // 30pixels for title
            // TODO: - icon
            //			rc_tit.w -= 30;
            //			rc_tit.x += 30;
            //title
            render_text_align(rc_tit, window->_ths->title, window->font_title,
                window->color_back, window->color_text, window->padding, ALIGN_CENTER);

            window->win.f_invalid = 0;
            window->flags |= DLG_DRA_FR | DLG_PHA_CH | DLG_PPR_CH;
        }
        //DLG_PHA_CH == DLG_TXT_CH | DLG_BTN_CH
        if (window->flags & DLG_TXT_CH) //text changed
        {
            _window_dlg_statemachine_draw_phase_text(window);
            window->flags &= ~DLG_TXT_CH;
        }
        if (window->flags & DLG_BTN_CH) //button changed
        {
            window->_ths->p_states[window->vars.phase].p_button->draw_cb(window);
            window->flags &= ~DLG_BTN_CH;
        }
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

void window_dlg_statemachine_event(window_dlg_statemachine_t *window,
    uint8_t event, void *param) {
    window_event_dlg_cb_t *event_cb = window->_ths->p_states[window->vars.phase].p_button->event_cb;
    if (event_cb != NULL)
        event_cb(window, event, param);
}

void window_dlg_statemachine_event_1bt(window_dlg_statemachine_t *window,
    uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    //case WINDOW_EVENT_BTN_UP:
    case WINDOW_EVENT_CLICK:
        window->vars.flags |= DLG_BT_FLG;
        return;
    }
}

const window_class_dlg_statemachine_t window_class_dlg_statemachine = {
    {
        WINDOW_CLS_USER,
        sizeof(window_dlg_statemachine_t),
        (window_init_t *)window_dlg_statemachine_init,
        0,
        (window_draw_t *)window_dlg_statemachine_draw,
        (window_event_t *)window_dlg_statemachine_event,
    },
};

#define LD_BT_DONE     DLG_DI_US0 //continue button for marlin
#define LD_BT_PURG     DLG_DI_US1 //resume   button for marlin
#define LD_BT_PURG_SEL DLG_DI_US2 //when flag is 0 active button is done

/*****************************************************************************/
//buttons
// is_donelhs_purgerhs == 1 == DONE  is on left hand side and PURGE is on right hand side
// is_donelhs_purgerhs == 0 == PURGE is on left hand side and DONE  is on right hand side
// does not effect texts - INTENDED, DO NOT CHANGE IT!!!
// left text is p_button->labels[0] an right is p_button->labels[1]
void window_dlg_load_draw_buttons(window_dlg_statemachine_t *window,
    int is_donelhs_purgerhs) {
    rect_ui16_t rc_btn = _get_dlg_statemachine_button_size(window);
    int is_enabled = window->_ths->p_states[window->vars.phase].p_button->flags & BT_ENABLED;

    const char *bt_caption = window->_ths->p_states[window->vars.phase].p_button->labels[0];
    int is_active = ((window->vars.flags & LD_BT_PURG_SEL) != 0) != (is_donelhs_purgerhs != 0);
    int16_t btn_width = rc_btn.w / 2 - gui_defaults.btn_spacing;

    rc_btn.w = btn_width;
    //lhs button
    button_draw(rc_btn, bt_caption, window->font_title, is_active && is_enabled);

    //more difficult calculations of coords to avoid round errors

    //space between buttons
    rc_btn.x += btn_width;
    rc_btn.w = _get_dlg_statemachine_button_size(window).w - rc_btn.w * 2;
    display->fill_rect(rc_btn, window->color_back);

    //distance of both buttons from screen sides is same
    rc_btn.x += rc_btn.w;
    rc_btn.w = btn_width;
    is_active = !is_active;
    bt_caption = window->_ths->p_states[window->vars.phase].p_button->labels[1];
    //rhs button
    button_draw(rc_btn, bt_caption, window->font_title, is_active && is_enabled);
}

void window_dlg_load_draw_buttons_cb(window_dlg_statemachine_t *window) {
    window_dlg_load_draw_buttons(window, 1);
};

//have to clear handled events
void window_dlg_load_event_cb(window_dlg_statemachine_t *window, uint8_t event, void *param) {
    uint8_t *p_flags = &window->vars.flags;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        if ((*p_flags) & LD_BT_PURG_SEL)
            window->vars.flags |= LD_BT_PURG; //set purge button
        else
            window->vars.flags |= LD_BT_DONE; //set done  button
        return;
    case WINDOW_EVENT_ENC_DN:
        if ((*p_flags) & LD_BT_PURG_SEL) {
            (*p_flags) &= ~LD_BT_PURG_SEL;
            set_repaint_btn_flg(window); //set window flag - wil repaint buttons
        }
        return;
    case WINDOW_EVENT_ENC_UP:
        if (!((*p_flags) & LD_BT_PURG_SEL)) {
            (*p_flags) |= LD_BT_PURG_SEL;
            set_repaint_btn_flg(window); //set window flag - wil repaint buttons
        }
        return;
    }
}

//todo copy - paste (safer before release) fixme
//have to clear handled events
void window_dlg_load_event_inverted_cb(window_dlg_statemachine_t *window, uint8_t event, void *param) {
    uint8_t *p_flags = &window->vars.flags;
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        if ((*p_flags) & LD_BT_PURG_SEL)
            window->vars.flags |= LD_BT_PURG; //set purge button
        else
            window->vars.flags |= LD_BT_DONE; //set done  button
        return;
    case WINDOW_EVENT_ENC_UP:
        if ((*p_flags) & LD_BT_PURG_SEL) {
            (*p_flags) &= ~LD_BT_PURG_SEL;
            set_repaint_btn_flg(window); //set window flag - wil repaint buttons
        }
        return;
    case WINDOW_EVENT_ENC_DN:
        if (!((*p_flags) & LD_BT_PURG_SEL)) {
            (*p_flags) |= LD_BT_PURG_SEL;
            set_repaint_btn_flg(window); //set window flag - wil repaint buttons
        }
        return;
    }
}

/*****************************************************************************/
//buttons
const float ld_purge_amount = 40.0F; //todo is this amount correct?

static const char *txt_stop[] = { "STOP" };
static const char *txt_cont[] = { "CONTINUE" };
static const char *txt_disa[] = { "DISABLE SENSOR" };
static const char *txt_none[] = { "" };
static const char *txt_yesno[] = { "YES", "NO" };

const _dlg_button_t bt_yesno_ena = {
    txt_yesno, BT_ENABLED, window_dlg_load_draw_buttons_cb, window_dlg_load_event_cb
};

const _dlg_button_t bt_yesno_dis = {
    txt_yesno, 0, window_dlg_load_draw_buttons_cb, NULL
};

const _dlg_button_t bt_stop_ena = {
    txt_stop, BT_ENABLED | BT_AUTOEXIT,
    window_dlg_statemachine_draw_1bt, window_dlg_statemachine_event_1bt
};

const _dlg_button_t bt_stop_dis = {
    txt_stop, 0, window_dlg_statemachine_draw_1bt, NULL
};

const _dlg_button_t bt_cont_ena = {
    txt_cont, BT_ENABLED,
    window_dlg_statemachine_draw_1bt, window_dlg_statemachine_event_1bt
};

const _dlg_button_t bt_disable_ena = {
    txt_disa, BT_ENABLED,
    window_dlg_statemachine_draw_1bt, window_dlg_statemachine_event_1bt
};

const _dlg_button_t bt_cont_dis = {
    txt_cont, 0, window_dlg_statemachine_draw_1bt, NULL
};

const _dlg_button_t bt_none = {
    txt_none, 0, window_dlg_statemachine_draw_0bt, NULL
};

const _dlg_state test_states[] = {
    { window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena },
    { window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena },
    { window_dlg_statemachine_draw_progress_tot, "Preparing to ram", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Ramming", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Press CONTINUE and\npush filament into\nthe extruder.     ", &bt_cont_ena },
    { window_dlg_statemachine_draw_progress_tot, "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ", &bt_cont_dis },
    { window_dlg_statemachine_draw_progress_tot, "Inserting", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Loading to nozzle", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Purging", &bt_stop_dis },
    { window_dlg_statemachine_draw_progress_tot, "Purging", &bt_none },
    { window_dlg_statemachine_draw_progress_none, "Is color correct?", &bt_yesno_ena }, //can end (state += 2)
    { window_dlg_statemachine_draw_progress_tot,                                        //was part
        "Purging", &bt_yesno_dis },                                                     //can jump back (state --)
};

/*
const _dlg_state test_states[] = {
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena },
    { 2000, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena },
    { 1500, window_dlg_statemachine_draw_progress_tot, "Preparing to ram", &bt_stop_dis },
    { 1500, window_dlg_statemachine_draw_progress_tot, "Ramming", &bt_stop_dis },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis },
    { 0, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis },
    { 0, window_dlg_statemachine_draw_progress_tot, "Press CONTINUE and\npush filament into\nthe extruder.     ", &bt_cont_ena },
    { 0, window_dlg_statemachine_draw_progress_tot, "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ", &bt_cont_dis },
    { 6000, window_dlg_statemachine_draw_progress_tot, "Inserting", &bt_stop_dis },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Loading to nozzle", &bt_stop_dis },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_stop_dis },
    { 0, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_none },
    { 0, window_dlg_statemachine_draw_progress_none, "Is color correct?", &bt_yesno_ena }, //can end (state += 2)
    { 0, window_dlg_statemachine_draw_progress_tot,//was part
    "Purging", &bt_yesno_dis },           //can jump back (state --)
};
*/
