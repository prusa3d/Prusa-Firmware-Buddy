// window_dlg_load_unload.hpp

#pragma once
#include "dlg_result.hpp"
#include "i18n.h"

dlg_result_t gui_dlg_load(string_view_utf8 caption);

dlg_result_t gui_dlg_purge(string_view_utf8 caption);

dlg_result_t gui_dlg_load_forced(void); //no return option

dlg_result_t gui_dlg_unload(string_view_utf8 caption);

dlg_result_t gui_dlg_unload_forced(void); //no return option + no skipping preheat
