// window_dlg_load.c
// does not have header
// window_dlg_loadunload.h used instead

#include "window_dlg_load.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"
#include "gui.h" //gui_defaults
#include "button_draw.h"
#include "filament.h"
#include "filament_sensor.h"

static const _cl_dlg cl_load;

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
}

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

static dlg_result_t _gui_dlg_load(void) {
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 30;
    dlg_result_t ret = _gui_dlg(&cl_load, &ld_vars, 600000); //10min
    if (ret != DLG_OK)set_filament(FILAMENT_NONE);
    return ret;
}

dlg_result_t gui_dlg_load(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat(NULL) < 1)
        return DLG_ABORTED; //0 is return
    return _gui_dlg_load();
}

dlg_result_t gui_dlg_load_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_forced("PREHEAT for LOAD") < 0)
        return DLG_ABORTED; //DLG_ABORTED should not happen
    return _gui_dlg_load();
}

/*****************************************************************************/
//LOAD
static int f_LD_GCODE(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    marlin_gcode("M701 Z0");
    p_vars->phase++;
    p_vars->flags |= DLG_CH_CMD; //kill screen after M701 ends
    return 0;
}

static int f_LD_INSERT_FILAMENT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    if (p_vars->flags & DLG_BT_FLG) {
        p_vars->flags &= ~DLG_BT_FLG;
        if (fs_get_state() == FS_NO_FILAMENT) p_vars->phase++; // f_CH_FILAMENT_SENSOR
        else  p_vars->phase += 2;//skip f_CH_FILAMENT_SENSOR
    }
    return 0;
}

static int f_LD_FILAMENT_SENSOR(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    if(fs_get_state() != FS_NO_FILAMENT) {
        p_vars->flags &= ~DLG_BT_FLG;//clr btn to be safe
        p_vars->phase--;
    }
    return 0;
}

static int f_LD_WAIT_E_POS__INSERTING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //todo i should not need this
    if (marlin_event_clr(MARLIN_EVT_UserConfirmRequired)) //clear wait for user
    {
        marlin_set_wait_user(0);
    }
    //wait E pos >= 40
    if (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= filament_change_slow_load_length)
        p_vars->phase++;
    return 100 * (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / filament_change_slow_load_length;
}

static int f_LD_WAIT_E_POS__LOADING_TO_NOZ(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //wait E pos >= 360
    if (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= filament_change_full_load_length)
        p_vars->phase++;
    float ret = 100 * (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - filament_change_slow_load_length) / (filament_change_full_load_length - filament_change_slow_load_length);
    return ret;
}

static int f_LD_WAIT_E_POS__PURGING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //wait E pos >= 400
    if ((additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] >= filament_change_full_purge_load_length) && (marlin_motion() == 0))
        p_vars->phase++;
    return 100 * (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - filament_change_full_load_length) / (filament_change_full_purge_load_length - filament_change_full_load_length);
}

static int f_LD_CHECK_MARLIN_EVENT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    if (marlin_event_clr(MARLIN_EVT_HostPrompt))
        p_vars->phase++;
    return 0;
}

static int f_LD_PURGE_USER_INTERACTION(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    switch (p_vars->flags & (LD_BT_PURG | LD_BT_DONE)) {
    case LD_BT_PURG:
        marlin_host_button_click(HOST_PROMPT_BTN_PurgeMore);
        additional_vars->e_last = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]; //needed in next phase
        p_vars->phase++; //f_LD_PURGE_SHOW_PROGRESS
        break;
    case LD_BT_DONE:
    case (LD_BT_DONE | LD_BT_PURG): //if both buttons are clicked DONE has priority, but should not happen
        marlin_host_button_click(HOST_PROMPT_BTN_Continue);
        p_vars->phase += 2; //DONE
        break;
    }
    return 100; //progressbar MUST be 100
}

static int f_LD_PURGE_SHOW_PROGRESS(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {

    if (marlin_event_clr(MARLIN_EVT_HostPrompt)) {
        p_vars->phase--; //jump back to f_LD_PURGE_USER_INTERACTION
        p_vars->flags &= (~(LD_BT_PURG | LD_BT_DONE)); //clr buttons
        p_vars->flags |= LD_BT_PURG_SEL; //select done
        return 0;
    }

    float ret = 100.0F * (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_last)
        / ld_purge_amount;
    if (ret > 99.0F)
        return 99;
    return (int)ret;
}

/*****************************************************************************/
//buttons
static const char *txt_yesno[] = { "YES", "NO" };
const _dlg_button_t bt_yesno_ena = {
    txt_yesno, BT_ENABLED, window_dlg_load_draw_buttons_cb, window_dlg_load_event_cb
};

const _dlg_button_t bt_yesno_dis = {
    txt_yesno, 0, window_dlg_load_draw_buttons_cb, NULL
};

/*****************************************************************************/
//LOAD
//name of func serves as comment
static const _dlg_state load_states[] = {
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 3000, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    { 0, window_dlg_statemachine_draw_progress_tot, "Press CONTINUE and\npush filament into\nthe extruder.     ", &bt_cont_ena, (dlg_state_func)f_LD_INSERT_FILAMENT },
	{ 0, window_dlg_statemachine_draw_progress_tot, "Make sure the     \nfilament is       \ninserted through  \nthe sensor.       ", &bt_cont_dis, (dlg_state_func)f_LD_FILAMENT_SENSOR },
	{ 0, window_dlg_statemachine_draw_progress_tot, "Inserting", &bt_stop_dis, (dlg_state_func)f_LD_GCODE },
    { 6000, window_dlg_statemachine_draw_progress_tot, "Inserting", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__INSERTING },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Loading to nozzle", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__LOADING_TO_NOZ },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_stop_dis, (dlg_state_func)f_LD_WAIT_E_POS__PURGING },
    { 0, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_none, (dlg_state_func)f_LD_CHECK_MARLIN_EVENT },
    { 0, window_dlg_statemachine_draw_progress_none, "Is color correct?", &bt_yesno_ena, (dlg_state_func)f_LD_PURGE_USER_INTERACTION }, //can end (state += 2)
    { 0, window_dlg_statemachine_draw_progress_part, "Purging", &bt_yesno_dis, (dlg_state_func)f_LD_PURGE_SHOW_PROGRESS }, //can jump back (state --)
};

static const _cl_dlg cl_load = {
    "Loading filament", //title
    load_states, //p_states
    sizeof(load_states) / sizeof(load_states[0]), //count
    f_SH_on_load, //on_load
    (dlg_loop_cb_t)f_SH_on_loop, //on_loop
    f_SH_on_timeout, //on_timeout
    NULL, //on_done
};
