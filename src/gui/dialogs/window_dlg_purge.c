// window_dlg_purge.c

#include "window_dlg_statemachine.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include "window_dlg_preheat.h"
#include "window_dlg_loadunload_shared.h"

extern const float ld_purge_amount;

static const _cl_dlg cl_purge;

void window_dlg_purge_draw_buttons_cb(window_dlg_statemachine_t *window) {
    window_dlg_load_draw_buttons(window, 0);
}

static dlg_result_t _gui_dlg_purge(void) {
    _dlg_ld_vars ld_vars;
    memset(&ld_vars, '\0', sizeof(ld_vars));
    ld_vars.z_min_extr_pos = 30;
    return _gui_dlg(&cl_purge, &ld_vars, 600000); //10min
}

dlg_result_t gui_dlg_purge(void) {
    //todo must be called inside
    if (gui_dlg_preheat_autoselect_if_able(NULL) < 1)
        return DLG_ABORTED; //user can choose "RETURN"
    return _gui_dlg_purge();
}

/*****************************************************************************/
//PURGE
//name of func serves as comment
static int f_PU_GCODE(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    marlin_gcode("M83"); //extruder relative mode
    marlin_gcode("G21"); //set units to millimeters
    p_vars->phase++;
    return 0;
}

static int f_PU_PURGE_USER_INTERACTION(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    switch (p_vars->flags & (LD_BT_PURG | LD_BT_DONE)) {
    case LD_BT_PURG:
        additional_vars->e_last = additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E]; //needed in next phase
        marlin_gcode("G1 E40 F200");
        p_vars->phase++; //f_LD_PURGE_SHOW_PROGRESS
        break;
    case LD_BT_DONE:
    case (LD_BT_DONE | LD_BT_PURG): //if both buttons are clicked DONE has priority, but should not happen
        p_vars->phase += 2; //DONE
        break;
    }
    return 100; //progressbar MUST be 100
}

static int f_PU_PURGE_SHOW_PROGRESS(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    float ret = 100.0F * (additional_vars->p_marlin_vars->pos[MARLIN_VAR_INDEX_E] - additional_vars->e_last)
        / ld_purge_amount;
    if (ret > 99.0F) {
        p_vars->phase--; //jump back to f_LD_PURGE_USER_INTERACTION
        p_vars->flags &= (~(LD_BT_PURG | LD_BT_DONE | LD_BT_PURG_SEL)); //clr buttons, select done
        return 100;
    }
    return (int)ret;
}

static int f_PU_WAIT_READY(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //wait ready
    if (!marlin_busy())
        p_vars->phase = -1;
    return 0;
}

static int f_PU_PREPICK_PURGE_BTN(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    p_vars->flags |= LD_BT_PURG_SEL; // default button is purge
    p_vars->phase++;
    return 0;
}

/*****************************************************************************/
//buttons purge, done
static const char *txt_prgdn[] = { "PURGE", "DONE" };
const _dlg_button_t bt_prgdn_ena = {
    txt_prgdn, BT_ENABLED, window_dlg_purge_draw_buttons_cb, window_dlg_load_event_inverted_cb
};

const _dlg_button_t bt_prgdn_dis = {
    txt_prgdn, 0, window_dlg_purge_draw_buttons_cb, NULL
};

static const _dlg_state purge_states[] = {
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_INIT },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_MOVE_INITIAL_Z },
    { 0, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_MOTION },
    { 3000, window_dlg_statemachine_draw_progress_tot, "Parking", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_INITIAL_Z_STOPPED },
    { 10000, window_dlg_statemachine_draw_progress_tot, "Waiting for temp.", &bt_stop_ena, (dlg_state_func)f_SH_WAIT_TEMP },
    { 0, window_dlg_statemachine_draw_progress_tot, "Purging", &bt_none, (dlg_state_func)f_PU_GCODE },
    { 0, window_dlg_statemachine_draw_progress_none, "Purge filament?", &bt_none, (dlg_state_func)f_PU_PREPICK_PURGE_BTN },
    { 0, window_dlg_statemachine_draw_progress_none, "Purge filament?", &bt_prgdn_ena, (dlg_state_func)f_PU_PURGE_USER_INTERACTION }, //can end (state += 2)
    { 0, window_dlg_statemachine_draw_progress_part, "Purging", &bt_prgdn_ena, (dlg_state_func)f_PU_PURGE_SHOW_PROGRESS }, //can jump back (state --)
    { 0, window_dlg_statemachine_draw_progress_part, "Finished", &bt_none, (dlg_state_func)f_PU_WAIT_READY }
};

static void purge_done(void) {
    marlin_gcode("M84"); // disable motors
}

//begin of each cycle
static loop_result_t purge_loop(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars) {
    //todo do I need all those variables?
    additional_vars->p_marlin_vars = marlin_update_vars(
        MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
    return LOOP_RESULT_CONTINUE;
}

static const _cl_dlg cl_purge = {

    "Purge nozzle", //title
    purge_states, //p_states
    sizeof(purge_states) / sizeof(purge_states[0]), //count
    //load/unload dialogs reacts to MARLIN_EVT_CommandEnd, purge dialog does not
    NULL, //on_load event,
    (dlg_loop_cb_t)purge_loop, //on_loop
    NULL, //on_timeout
    purge_done, //on_done
};
