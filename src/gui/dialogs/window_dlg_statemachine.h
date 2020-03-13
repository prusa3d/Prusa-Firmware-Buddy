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
    //uint8_t part_progress;
    //uint8_t prev_part_progress;
    //uint32_t tick_part_start;
    //uint32_t time_total;
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
    const _dlg_button_t *p_button;
} _dlg_state;

typedef struct
{
    const char *title;
    const _dlg_state *p_states;
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

extern const window_class_dlg_statemachine_t window_class_dlg_statemachine;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_DLG_STATEMACHINE_H
