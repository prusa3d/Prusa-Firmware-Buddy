#include "DialogStateful.hpp"
#include "DialogRadioButton.hpp"
#include "window_dlg_statemachine.h"
#include "gui.h"

#include "display_helper.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "button_draw.h"
#include "window_dlg_change.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_msgbox.h"

//#define DLG_FRAME_ENA 1
#define DLG_FRAME_ENA 0

//dialog flags bitshift
#define DLG_SHI_MOD 4  // mode shift
#define DLG_SHI_CHG 14 // change flag shift

#if DLG_FRAME_ENA == 1
    #define DLG_DRA_FR 0x0800 // draw frame
#else
    #define DLG_DRA_FR 0x0000                // draw frame
#endif                                       //DLG_FRAME_ENA == 1
#define DLG_TXT_CH 0x2000                    // text changed
#define DLG_PRO_CH 0x4000                    // progress changed
#define DLG_PPR_CH 0x8000                    // part progress changed
#define DLG_PRX_CH (DLG_PRO_CH | DLG_PPR_CH) // some progress changed
#define DLG_PHA_CH (DLG_PRX_CH | DLG_TXT_CH) // phase changed
//dialog flags bitmasks
#define DLG_MSK_MOD 0x0003     // mode mask
#define DLG_MSK_CHG DLG_PHA_CH // change flag mask

//button flags
//combination of enabled and not visible  == do not clear
#define BT_ENABLED ((uint8_t)(1 << 0))
//#define BT_VISIBLE  ((uint8_t)(1 << 1))
#define BT_AUTOEXIT ((uint8_t)(1 << 2))

#define DLG_CH_CMD ((uint8_t)(1 << 1)) //check marlin_command()

//flags for draw_cb function (user callback)
#define DLG_DI_US0 ((uint8_t)(1 << 4)) //user flag 0
#define DLG_DI_US1 ((uint8_t)(1 << 5)) //user flag 1
#define DLG_DI_US2 ((uint8_t)(1 << 6)) //user flag 2
#define DLG_DI_US3 ((uint8_t)(1 << 7)) //user flag 3

extern window_t *window_1; //current popup window, C-code remain

typedef struct _window_dlg_statemachine_t window_dlg_statemachine_t;

typedef void(window_draw_dlg_cb_t)(window_dlg_statemachine_t *window);
//this type does not match to window_event_t .. p_event is pointer
typedef void(window_event_dlg_cb_t)(window_dlg_statemachine_t *window, uint8_t event, void *param);

#pragma pack(push)
#pragma pack(1)

//universal dialog vars
typedef struct
{
    //uint8_t flags;
    int8_t phase;
    int8_t prev_phase;
    uint8_t progress;
    uint8_t prev_progress;
} _dlg_vars;

typedef struct
{
    const char **labels;
    uint8_t flags;
    window_draw_dlg_cb_t *draw_cb;
    window_event_dlg_cb_t *event_cb;
} _dlg_button_t;

typedef struct
{
    window_draw_dlg_cb_t *progress_draw;
    const char *text;
    //const _dlg_button_t *p_button;
    RadioButton radio_btn;
} _dlg_state;

typedef struct
{
    const char *title;
    _dlg_state *p_states;
    const size_t count;

} _cl_dlg;

typedef struct _window_dlg_statemachine_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint16_t flags;
    uint8_t last_text_h; //hack todo remove me

    _cl_dlg *_ths;
    _dlg_vars vars;
} window_dlg_statemachine_t;

#pragma pack(pop)

extern _dlg_state test_states[14];

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

int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;

extern window_t *window_1; //current popup window

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
/*
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
*/
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

void window_dlg_statemachine_event(window_dlg_statemachine_t *window,
    uint8_t event, void *param) {
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

/*****************************************************************************/
//buttons
const float ld_purge_amount = 40.0F; //todo is this amount correct?

static const PhaseTexts txt_stop = { "STOP", "", "", "" };
static const PhaseTexts txt_cont = { "CONTINUE", "", "", "" };
static const PhaseTexts txt_disa = { "DISABLE SENSOR", "", "", "" };
static const PhaseTexts txt_none = { "", "", "", "" };
static const PhaseTexts txt_yesno = { "YES", "NO", "", "" };

rect_ui16_t _get_dlg_statemachine_button_size() {
    rect_ui16_t rc_btn = rect_ui16(0, 0, display->w, display->h);
    rc_btn.y += (rc_btn.h - 40); // 30pixels for button (+ 10 space for grey frame)
    rc_btn.h = 30;
    rc_btn.x += 6;
    rc_btn.w -= 12;
    return rc_btn;
}

static const RadioButton::window_t radio_win = { gui_defaults.font_big, gui_defaults.color_back, _get_dlg_statemachine_button_size() };

_dlg_state test_states[] = {
    { window_dlg_statemachine_draw_progress_tot, "Parking", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Parking), txt_stop, true) },
    { window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::WaitingTemp), txt_stop, true) },
    { window_dlg_statemachine_draw_progress_tot, "Preparing to ram", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::PreparingToRam), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Ramming", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Ramming), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Unloading", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Unloading), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Unloading", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Unloading2), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Press CONTINUE and\npush filament into\nthe extruder.     ", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::UserPush), txt_cont, true) },
    { window_dlg_statemachine_draw_progress_tot, "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::MakeSureInserted), txt_cont, false) },
    { window_dlg_statemachine_draw_progress_tot, "Inserting", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Inserting), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Loading to nozzle", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Loading), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Purging", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging), txt_stop, false) },
    { window_dlg_statemachine_draw_progress_tot, "Purging", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging2), txt_none, false) },
    { window_dlg_statemachine_draw_progress_none, "Is color correct?", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::IsColor), txt_yesno, true) }, //can end (state += 2)
    { window_dlg_statemachine_draw_progress_tot,                                                                                                                          //was part progress
        "Purging", RadioButton(radio_win, DialogCommands::GetCommands(PhasesLoadUnload::Purging3), txt_yesno, false) },                                                   //can jump back (state --)
};
