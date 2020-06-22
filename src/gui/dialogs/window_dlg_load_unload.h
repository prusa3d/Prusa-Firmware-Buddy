// window_dlg_load_unload.h

#pragma once
#include "dlg_result.h"

extern dlg_result_t gui_dlg_load(void);

extern dlg_result_t gui_dlg_purge(void);

extern dlg_result_t gui_dlg_load_forced(void); //no return option

extern dlg_result_t gui_dlg_unload(void);

extern dlg_result_t gui_dlg_unload_forced(void); //no return option + no skipping preheat
