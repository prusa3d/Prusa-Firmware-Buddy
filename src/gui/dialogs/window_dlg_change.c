// window_dlg_change.c

#include "gui.h"
#include "window_dlg_statemachine.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_msgbox.h"
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"
#include "button_draw.h"
#include "filament_sensor.h"

static const _cl_dlg cl_unload;

static dlg_result_t _gui_dlg_change(void) {
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 10;
    dlg_result_t res = _gui_dlg(&cl_unload, &ld_vars, -1); //-1 == 49710 days
    if (res == DLG_OK) {
        if (fs_get_state() == FS_NO_FILAMENT) {
            if (gui_msgbox("The filament sensor failed to detect inserted filament. Disable the sensor?",
                    MSGBOX_BTN_YESNO | MSGBOX_ICO_QUESTION)
                == MSGBOX_RES_YES)
                fs_disable();
            else
                fs_enable();
        }
    }
    return res;
}

dlg_result_t gui_dlg_change(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    //if (gui_dlg_preheat_autoselect_if_able(NULL) < 1) return DLG_ABORTED;//user can choose "RETURN"
    return _gui_dlg_change();
}

/*****************************************************************************/
//CHANGE - unload

//ram sequence .. kinda glitchy
// 0 ... -8  ... 8 ... -392
static int f_CH_WAIT_E_POS__RAM_RETRACTING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // 0 ... -8
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    if ( //todo unprecise
        (pos < -7.9F) || (pos > 0.0F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (-pos) / 8;
    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_CH_WAIT_E_POS__RAMMING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // -8  ... 8
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    if ( //todo unprecise
        (pos > 4.0F) || (pos < -20) // skip this phase - this can maybe happend when we miss previous condition
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 7.9F) ||
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -8.1F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (8.0F + pos) / (8 + 8);

    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_CH_WAIT_E_POS__UNLOADING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // 8 ... -392
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    if (pos < (filament_unload_mini_length - 1.0F))
        p_vars->phase++;
    return 100 * (8.0F - pos) / (filament_unload_mini_length + 8);
}

static int f_CH_INIT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    marlin_event_clr(MARLIN_EVT_CommandEnd);
    p_vars->flags |= DLG_CH_CMD; //kill screen after M600 ends

    //marlin_gcode("G92 E0");//saves calculation
    additional_vars->z_start = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_Z))->pos[2];
    if ((marlin_vars()->motion & 0x08) == 0) {
        additional_vars->e_start = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_E))->pos[3];
        p_vars->phase++;
    }
    return 0;
}

/*****************************************************************************/
//buttons
extern void window_dlg_load_draw_buttons_cb(window_dlg_statemachine_t *window);
extern void window_dlg_load_event_cb(window_dlg_statemachine_t *window, uint8_t event, void *param);
extern const _dlg_button_t bt_yesno_ena;
extern const _dlg_button_t bt_yesno_dis;

/*****************************************************************************/
//CHANGE - load

static int f_CH_INSERT_FILAMENT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    if (p_vars->flags & DLG_BT_FLG) {
        p_vars->flags &= ~DLG_BT_FLG;
        p_vars->phase++;
        additional_vars->e_start = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_E))->pos[3];
    }
    return 0;
}

static int f_CH_WAIT_E_POS__INSERTING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //todo i should not need this
    if (marlin_event_clr(MARLIN_EVT_UserConfirmRequired)) //clear wait for user
    {
        marlin_set_wait_user(0);
    }
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    //wait E pos >= 40
    if (pos >= filament_change_slow_load_length)
        p_vars->phase++;
    return 100 * (pos) / filament_change_slow_load_length;
}

static int f_CH_WAIT_E_POS__LOADING_TO_NOZ(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //wait E pos >= 360
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    if (pos >= filament_change_full_load_length)
        p_vars->phase++;
    float ret = 100 * (pos - filament_change_slow_load_length) / (filament_change_full_load_length - filament_change_slow_load_length);
    return ret;
}

static int f_CH_WAIT_E_POS__PURGING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //wait E pos >= 400
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_start;
    if ((pos >= filament_change_full_purge_load_length) && (marlin_motion() == 0))
        p_vars->phase++;
    return 100 * (pos - filament_change_full_load_length) / (filament_change_full_purge_load_length - filament_change_full_load_length);
}

static int f_CH_CHECK_MARLIN_EVENT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    if (marlin_event_clr(MARLIN_EVT_HostPrompt))
        p_vars->phase++;
    return 0;
}

static int f_CH_PURGE_USER_INTERACTION(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
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

static int f_CH_PURGE_SHOW_PROGRESS(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {

    if (marlin_event_clr(MARLIN_EVT_HostPrompt)) {
        p_vars->phase--; //jump back to f_LD_PURGE_USER_INTERACTION
        p_vars->flags &= (~(LD_BT_PURG | LD_BT_DONE)); //clr buttons
        p_vars->flags |= LD_BT_PURG_SEL; //select done
        return 0;
    }
    float pos = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E];
    float ret = 100.0F * (pos - additional_vars->e_last)
        / ld_purge_amount;
    if (ret > 99.0F)
        return 99;
    return (int)ret;
}

/*****************************************************************************/
//CHANGE
//name of func serves as comment
static const _dlg_state unload_states[] = {
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_CH_INIT },
    //	{    0,window_dlg_statemachine_draw_progress_tot, "Parking",           &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z},
    //	{    0,window_dlg_statemachine_draw_progress_tot, "Parking",           &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION},
    //	{ 1000,window_dlg_statemachine_draw_progress_tot, "Parking",           &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED},
    { 2000, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    //	{    0,window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_dis, (dlg_state_func)f_UL_GCODE},
    { 1500, window_dlg_statemachine_draw_progress_tot, "Preparing to ram", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__RAM_RETRACTING },
    { 1500, window_dlg_statemachine_draw_progress_tot, "Ramming", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__RAMMING },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__UNLOADING },
    { 0, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis, (dlg_state_func)f_SH_WAIT_E_STOPPED },
    { 0, window_dlg_statemachine_draw_progress_tot, "Press CONTINUE and\npush filament into\nthe extruder.", &bt_cont_ena, (dlg_state_func)f_CH_INSERT_FILAMENT },
    { 6000, window_dlg_statemachine_draw_progress_tot, "Inserting", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__INSERTING },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Loading to nozzle", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__LOADING_TO_NOZ },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_stop_dis, (dlg_state_func)f_CH_WAIT_E_POS__PURGING },
    { 0, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_none, (dlg_state_func)f_CH_CHECK_MARLIN_EVENT },
    { 0, window_dlg_statemachine_draw_progress_none, "Is color correct?", &bt_yesno_ena, (dlg_state_func)f_CH_PURGE_USER_INTERACTION }, //can end (state += 2)
    { 0, window_dlg_statemachine_draw_progress_part, "Purging", &bt_yesno_dis, (dlg_state_func)f_CH_PURGE_SHOW_PROGRESS }, //can jump back (state --)
};

static const _cl_dlg cl_unload = {
    "Change filament", //title
    unload_states, //p_states
    sizeof(unload_states) / sizeof(unload_states[0]), //count
    f_SH_on_load, //on_load
    (dlg_loop_cb_t)f_SH_on_loop, //on_loop
    f_SH_on_timeout, //on_timeout
    NULL, //on_done
};
