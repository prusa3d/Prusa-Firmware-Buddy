// window_dlg_unload.h

#ifndef _WINDOW_DLG_UNLOAD_H
#define _WINDOW_DLG_UNLOAD_H

#include "dlg_result.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern dlg_result_t gui_dlg_unload(void);

extern dlg_result_t gui_dlg_unload_forced(void); //no return option + no skipping preheat

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_DLG_UNLOAD_H
