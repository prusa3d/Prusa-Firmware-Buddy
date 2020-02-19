// window_dlg_statemachine.c
#include "window_dlg_statemachine.h"
#include "display_helper.h"
#include "gui.h"
#include "dbg.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "button_draw.h"

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

int16_t WINDOW_CLS_DLG_LOADUNLOAD = 0;

extern window_t *window_1; //current popup window

static uint32_t _phase_time_total(int phase, const _cl_dlg *_ths);

static void clr_logs();
static void is_part_log(int part_progress);
static void progress_changed_log(int progress);
static void phase_changed_log(int phase, int base_progress);
static void print_log();

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
    window->vars.prev_progress = 0;
    window->vars.part_progress = 0;
    window->vars.base_progress = 0;
    window->flags = 0;

    window->vars.tick_part_start = HAL_GetTick();
    window->vars.time_total = _phase_time_total(window->_ths->count - 1, window->_ths);
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

void window_dlg_statemachine_draw_progress_part(window_dlg_statemachine_t *window) {
    if (window->flags & DLG_PPR_CH)
        progress_draw(window->win.rect, window->font_title, window->color_back,
            window->color_text, window->padding, window->vars.part_progress);
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

dlg_result_t _gui_dlg(const _cl_dlg *_ths, void *p_additional_vars, int32_t ttl) {
    clr_logs();

    window_dlg_statemachine_t dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg._ths = _ths;
    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_DLG_LOADUNLOAD, 0, gui_defaults.msg_box_sz, &dlg);
    window_1 = (window_t *)&dlg;
    gui_reset_jogwheel();
    gui_invalidate();
    window_set_capture(id);

    _dlg_vars *const p_vars = &(dlg.vars);

    uint32_t start_tick = HAL_GetTick();

    //inicialization cb given in parameter
    if (_ths->on_load != NULL)
        _ths->on_load();

    while ((size_t)(p_vars->phase) < _ths->count) //negative number will force exit
    {
        //loop cb given in parameter
        if (_ths->on_loop != NULL) {
            if (_ths->on_loop(p_vars, p_additional_vars) == LOOP_RESULT_BREAK)
                break;
        }

        //time to live reached?
        if ((uint32_t)(HAL_GetTick() - start_tick) >= (uint32_t)ttl) {
            if (_ths->on_timeout != NULL)
                _ths->on_timeout();
            break;
        }

        //button auto exit flag must be handled here
        //state could change phase
        if (
            (p_vars->flags & DLG_BT_FLG) &&                               //button clicked
            (_ths->p_states[p_vars->phase].p_button->flags & BT_AUTOEXIT) //autoexit flag on current button
        ) {
            break;
        }

        int part_progress = _ths->p_states[p_vars->phase].state_fnc(p_vars, p_additional_vars); //state machine

        //forced exit
        //intended to be used as handling  user clicked exit button
        //any state change phase to -1 and force exit
        if ((size_t)(p_vars->phase) >= _ths->count)
            break;

        p_vars->flags &= ~DLG_BT_FLG; //disable button clicked

        is_part_log(part_progress); //log raw value obtained from state

        //calculate partial progress (progress of current phase)
        if (part_progress > p_vars->part_progress)
            p_vars->part_progress = (uint8_t)part_progress; //progress can only rise
        if (part_progress > 100)
            p_vars->part_progress = 100;
        if (part_progress < 0)
            p_vars->part_progress = 0;

        if (p_vars->prev_part_progress != p_vars->part_progress) {
            dlg.flags |= (p_vars->prev_part_progress != p_vars->part_progress) ? DLG_PPR_CH : 0;
        }

        //phase changed
        if (p_vars->prev_phase != p_vars->phase) {
            p_vars->part_progress = 0;
            dlg.flags |= DLG_PHA_CH;
            gui_invalidate();
            p_vars->base_progress = 100 * _phase_time_total(p_vars->prev_phase, _ths) / p_vars->time_total;
            p_vars->prev_phase = p_vars->phase;
            p_vars->tick_part_start = HAL_GetTick();

            phase_changed_log(p_vars->phase, p_vars->base_progress);
        }

        //calculate "total" progress
        //p_vars->part_progress MUST remain unscaled
        //p_vars->base_progress is scaled
        part_progress = (int)(p_vars->part_progress) * (_ths->p_states[p_vars->phase].time) / p_vars->time_total;
        p_vars->progress = p_vars->base_progress + part_progress;

        //in last phase maximum progress is 100
        //in all other phases maximum progress is 99
        if ((size_t)(p_vars->phase) == _ths->count - 1) {
            if (p_vars->progress > 100)
                p_vars->progress = 100;
        } else {
            if (p_vars->progress > 99)
                p_vars->progress = 99;
        }

        //progress changed
        dlg.flags |= (p_vars->prev_progress != p_vars->progress) ? DLG_PRO_CH : 0;

        if (dlg.flags & (DLG_PRX_CH)) //any progress changed
        {
            gui_invalidate();
            progress_changed_log(p_vars->progress);
            p_vars->prev_progress = p_vars->progress;
        }

        print_log();
        gui_loop();
    }
    if (_ths->on_done != NULL)
        _ths->on_done();
    window_destroy(id);
    window_set_capture(id_capture);
    window_invalidate(0);
    return DLG_OK;
}

static uint32_t _phase_time_total(int phase, const _cl_dlg *_ths) {
    uint32_t time = 0;
    while (phase >= 0)
        time += _ths->p_states[phase--].time;
    return time;
}

/*****************************************************************************/
//DEBUG PRINT LOAD PROGRESS
#if DBG_LOAD_PROGRESS == 1

static int log_flag;
static int base_progress_log_min;
static int base_progress_log_max;
static int part_progress_log_min;
static int part_progress_log_max;
static int progress_log;
static int phase_log;
static marlin_vars_t *p_marlin_vars;

static void clr_logs() {
    p_marlin_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E));
    log_flag = 0;
    base_progress_log_min = INT_MAX;
    base_progress_log_max = INT_MIN;
    part_progress_log_min = INT_MAX;
    part_progress_log_max = INT_MIN;
    progress_log = -1;
    phase_log = -1;
}

static void is_part_log(int part_progress) {
    if (part_progress_log_min > part_progress) {
        part_progress_log_min = part_progress;
        log_flag = 1;
        //_dbg("part_progress_min[%d] = %d",phase, part_progress);
    }
    if (part_progress_log_max < part_progress) {
        part_progress_log_max = part_progress;
        log_flag = 1;
        //_dbg("part_progress_max[%d] = %d",phase, part_progress);
    }
}

static void progress_changed_log(int progress) {
    log_flag = 1;
    progress_log = progress;
}

static void phase_changed_log(int phase, int base_progress) {
    int _progress_log = progress_log; //save progress
    clr_logs();
    progress_log = _progress_log; //restore
    log_flag = 1;
    phase_log = phase;

    if (base_progress_log_min > base_progress) {
        base_progress_log_min = base_progress;
        log_flag = 1;
    }
    if (base_progress_log_max < base_progress) {
        base_progress_log_max = base_progress;
        log_flag = 1;
    }
}

static void print_log() {
    if (log_flag) {
        log_flag = 0;
        _dbg("phase = %d, progress = %d", phase_log, progress_log);
        _dbg("part_progress_min = %d", part_progress_log_min);
        _dbg("part_progress_max = %d", part_progress_log_max);
        _dbg("base_progress_min = %d", base_progress_log_min);
        _dbg("base_progress_max = %d", base_progress_log_max);
        _dbg("E pos = %f", (double)p_marlin_vars->pos[MARLIN_VAR_INDEX_E]);
        _dbg("Z pos = %f", (double)p_marlin_vars->pos[MARLIN_VAR_INDEX_Z]);
    }
}

#else //DBG_LOAD_PROGRESS
static void clr_logs() {}

static void is_part_log(int part_progress) {}

static void progress_changed_log(int progress) {}

static void phase_changed_log(int phase, int base_progress) {}

static void print_log() {}

#endif //DBG_LOAD_PROGRESS
