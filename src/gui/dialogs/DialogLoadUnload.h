#pragma once

#include "window.hpp"
#include "dlg_result.h"

struct window_class_dlg_statemachine_t {
    window_class_t cls;
};

extern int16_t WINDOW_CLS_DLG_LOADUNLOAD;
extern const window_class_dlg_statemachine_t window_class_dlg_statemachine;
