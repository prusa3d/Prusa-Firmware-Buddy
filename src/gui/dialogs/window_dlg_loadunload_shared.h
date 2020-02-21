// window_dlg_loadunload_shared.h

#ifndef _WINDOW_DLG_LOADUNLOAD_SHARED_H
#define _WINDOW_DLG_LOADUNLOAD_SHARED_H

#include "window_dlg_statemachine.h"
#include "marlin_client.h" //marlin_vars_t

#pragma pack(push)
#pragma pack(1)

//load unload dialog specific vars
typedef struct
{
    marlin_vars_t *p_marlin_vars;
    float z_min_extr_pos; //minimal z position for extruding
    float initial_move;
    float z_start;
    float e_start;
    float e_last; //todo use me more
} _dlg_ld_vars;

#pragma pack(pop)

/*****************************************************************************/
//shared for LOAD and UNLOAD

#define LD_BT_DONE     DLG_DI_US0 //continue button for marlin
#define LD_BT_PURG     DLG_DI_US1 //resume   button for marlin
#define LD_BT_PURG_SEL DLG_DI_US2 //when flag is 0 active button is done
//when flag is 1 active button is purge

extern const float ld_purge_amount;

extern const _dlg_button_t bt_stop_ena;
extern const _dlg_button_t bt_stop_dis;
extern const _dlg_button_t bt_cont_ena;
extern const _dlg_button_t bt_cont_dis;
extern const _dlg_button_t bt_disable_ena;
extern const _dlg_button_t bt_none;

extern void f_SH_on_load(void);                                                        //after start
extern loop_result_t f_SH_on_loop(_dlg_vars *p_vars, _dlg_ld_vars *p_additional_vars); //begin of each cycle
extern void f_SH_on_timeout();                                                         //on_timeout

extern int f_SH_INIT(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_MOVE_INITIAL_Z(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_WAIT_INITIAL_Z_MOTION(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_WAIT_INITIAL_Z_STOPPED(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_WAIT_E_MOTION(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_WAIT_E_STOPPED(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);
extern int f_SH_WAIT_TEMP(_dlg_vars *p_vars, _dlg_ld_vars *additional_vars);

//Externs from load
extern void window_dlg_load_draw_buttons(window_dlg_statemachine_t *window,
    int is_donelhs_purgerhs);

void window_dlg_load_event_cb(window_dlg_statemachine_t *window, uint8_t event, void *param);

void window_dlg_load_event_inverted_cb(window_dlg_statemachine_t *window, uint8_t event, void *param);

#endif //_WINDOW_DLG_LOADUNLOAD_SHARED_H
