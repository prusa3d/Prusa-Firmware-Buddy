#pragma once

#include "window.h"
#include "dlg_result.h"

typedef struct _window_class_dlg_statemachine_t {
    window_class_t cls;
} window_class_dlg_statemachine_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;
extern const window_class_dlg_statemachine_t window_class_dlg_statemachine;

#ifdef __cplusplus
}
#endif //__cplusplus
