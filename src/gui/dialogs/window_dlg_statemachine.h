// window_dlg_statemachine.h

#ifndef _WINDOW_DLG_STATEMACHINE_H
#define _WINDOW_DLG_STATEMACHINE_H

#include "window.h"
#include "dlg_result.h"

typedef struct _window_dlg_statemachine_t window_dlg_statemachine_t;

extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;

//button flags
//combination of enabled and not visible  == do not clear
#define BT_ENABLED ((uint8_t)(1 << 0))
//#define BT_VISIBLE  ((uint8_t)(1 << 1))
#define BT_AUTOEXIT ((uint8_t)(1 << 2))

#define DLG_BT_FLG ((uint8_t)(1 << 0)) //button flag
#define DLG_CH_CMD ((uint8_t)(1 << 1)) //check marlin_command()

//flags for draw_cb function (user callback)
#define DLG_DI_US0 ((uint8_t)(1 << 4)) //user flag 0
#define DLG_DI_US1 ((uint8_t)(1 << 5)) //user flag 1
#define DLG_DI_US2 ((uint8_t)(1 << 6)) //user flag 2
#define DLG_DI_US3 ((uint8_t)(1 << 7)) //user flag 3

typedef void(window_draw_dlg_cb_t)(window_dlg_statemachine_t *window);
//this type does not match to window_event_t .. p_event is pointer
typedef void(window_event_dlg_cb_t)(window_dlg_statemachine_t *window, uint8_t event, void *param);

#pragma pack(push)
#pragma pack(1)

//universal dialog vars
typedef struct
{
    uint8_t flags;
    int8_t phase;
    int8_t prev_phase;
    uint8_t progress;
    uint8_t prev_progress;
    uint8_t part_progress;
    uint8_t base_progress;
    uint8_t prev_part_progress;
    uint32_t tick_part_start;
    uint32_t time_total;
} _dlg_vars;

typedef enum {
    LOOP_RESULT_CONTINUE,
    LOOP_RESULT_BREAK
} loop_result_t;

typedef int (*dlg_state_func)(_dlg_vars *p_vars, void *p_additional_vars);          //prototype of state function
typedef void (*dlg_cb_t)(void);                                                     //dialog callback
typedef loop_result_t (*dlg_loop_cb_t)(_dlg_vars *p_vars, void *p_additional_vars); //dialog loop callback can break loop

typedef struct
{
    const char **labels;
    uint8_t flags;
    window_draw_dlg_cb_t *draw_cb;
    window_event_dlg_cb_t *event_cb;
} _dlg_button_t;

typedef struct
{
    uint32_t time;
    window_draw_dlg_cb_t *progress_draw;
    const char *text;
    const _dlg_button_t *p_button;
    dlg_state_func state_fnc;
} _dlg_state;

typedef struct
{
    const char *title;
    const _dlg_state *p_states;
    const size_t count;
    const dlg_cb_t on_load;      //after start
    const dlg_loop_cb_t on_loop; //begin of each cycle
    const dlg_cb_t on_timeout;   //check inside loop
    const dlg_cb_t on_done;      //before finish
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

    const _cl_dlg *_ths;
    _dlg_vars vars;
} window_dlg_statemachine_t;

typedef struct _window_class_dlg_statemachine_t {
    window_class_t cls;
} window_class_dlg_statemachine_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//flags are not directly accessible from outside
extern void set_repaint_btn_flg(window_dlg_statemachine_t *window);
extern int is_repaint_btn_flg(window_dlg_statemachine_t *window);

extern void window_dlg_statemachine_draw_progress_tot(window_dlg_statemachine_t *window);
extern void window_dlg_statemachine_draw_progress_part(window_dlg_statemachine_t *window);
extern void window_dlg_statemachine_draw_progress_none(window_dlg_statemachine_t *window);

extern rect_ui16_t _get_dlg_statemachine_button_size(window_dlg_statemachine_t *window);

extern dlg_result_t _gui_dlg(const _cl_dlg *_ths, void *p_additional_vars, int32_t ttl);

extern const window_class_dlg_statemachine_t window_class_dlg_statemachine;

//button draw mwthods
extern void window_dlg_statemachine_draw_0bt(window_dlg_statemachine_t *window);
extern void window_dlg_statemachine_draw_1bt(window_dlg_statemachine_t *window);
extern void window_dlg_statemachine_event_1bt(
    window_dlg_statemachine_t *window, uint8_t event, void *param);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_DLG_STATEMACHINE_H
