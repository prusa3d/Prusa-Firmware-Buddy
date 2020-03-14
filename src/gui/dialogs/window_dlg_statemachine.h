// window_dlg_statemachine.h

#ifndef _WINDOW_DLG_STATEMACHINE_H
#define _WINDOW_DLG_STATEMACHINE_H

#include "window.h"
#include "dlg_result.h"

extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;

#pragma pack(push)
#pragma pack(1)

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
