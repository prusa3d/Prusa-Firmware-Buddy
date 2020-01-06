// window_dlg_unload.c

#include "window_dlg_statemachine.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"
#include "filament.h"

static const _cl_dlg cl_unload;

static dlg_result_t _gui_dlg_unload(void) {
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 10;
    dlg_result_t ret = _gui_dlg(&cl_unload, &ld_vars, 300000); //5min
    set_filament(FILAMENT_NONE);
    return ret;

}

dlg_result_t gui_dlg_unload(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able(NULL) < 1)
        return DLG_ABORTED; //user can choose "RETURN"
    return _gui_dlg_unload();
}

dlg_result_t gui_dlg_unload_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able_forced("PREHEAT for UNLOAD") < 0)
        return DLG_ABORTED; //LD_ABORTED should not happen
    return _gui_dlg_unload();
}

/*****************************************************************************/
//UNLOAD

static int f_UL_GCODE(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    marlin_gcode("M702 Z0");
    p_vars->phase++;
    p_vars->flags |= DLG_CH_CMD; //kill screen after M702 ends
    return 0;
}

//ram sequence .. kinda glitchy
// 0 ... -8  ... 8 ... -392
static int f_UL_WAIT_E_POS__RAM_RETRACTING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // 0 ... -8
    if ( //todo unprecise
        (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -7.9F) || (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 0.0F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (-additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / 8;
    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_UL_WAIT_E_POS__RAMMING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // -8  ... 8
    if ( //todo unprecise
        additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 4.0F
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] > 7.9F) ||
        //(p_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -8.1F) //this part should not happen, just to be safe
    )
        p_vars->phase++;

    float ret = 100.0F * (8.0F + additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / (8 + 8);

    if (ret >= 100)
        return 100;
    if (ret <= 0)
        return 0;
    return ret;
}

static int f_UL_WAIT_E_POS__UNLOADING(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    // 8 ... -392
    if (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] < -391.9F)
        p_vars->phase++;
    return 100 * (8.0F - additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]) / (392 + 8);
}

/*****************************************************************************/
//UNLOAD
//name of func serves as comment
static const _dlg_state unload_states[] = {
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 1000, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    { 0, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_dis, (dlg_state_func)f_UL_GCODE },
    { 1500, window_dlg_statemachine_draw_progress_tot, "Preparing to ram", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__RAM_RETRACTING },
    { 1500, window_dlg_statemachine_draw_progress_tot, "Ramming", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__RAMMING },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis, (dlg_state_func)f_UL_WAIT_E_POS__UNLOADING },
    { 0, window_dlg_statemachine_draw_progress_tot, "Unloading", &bt_stop_dis, (dlg_state_func)f_SH_WAIT_E_STOPPED },
};

static const _cl_dlg cl_unload = {
    "Unloading filament", //title
    unload_states, //p_states
    sizeof(unload_states) / sizeof(unload_states[0]), //count
    f_SH_on_load, //on_load
    (dlg_loop_cb_t)f_SH_on_loop, //on_loop
    f_SH_on_timeout, //on_timeout
    NULL, //on_done
};
