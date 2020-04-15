// window_dlg_load_unload.h

#pragma once
#include "dlg_result.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern dlg_result_t gui_dlg_load(void);

extern dlg_result_t gui_dlg_load_forced(void); //no return option

extern dlg_result_t gui_dlg_unload(void);

extern dlg_result_t gui_dlg_unload_forced(void); //no return option + no skipping preheat

#ifdef __cplusplus
}
#endif //__cplusplus
