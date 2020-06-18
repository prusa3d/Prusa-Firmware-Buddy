// wizard_load_unload.h
#pragma once
#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"

typedef enum {
    LD_UNLD_INIT,
    LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD,
    LD_UNLD_DIALOG_PREHEAT,
    LD_UNLD_DIALOG_LOAD,
    LD_UNLD_DIALOG_UNLOAD,
    LD_UNLD_DONE
} LD_UNLD_STATE_t;

extern LD_UNLD_STATE_t wizard_load_unload(LD_UNLD_STATE_t state);
