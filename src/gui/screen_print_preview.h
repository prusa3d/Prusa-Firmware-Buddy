#pragma once
#include "gui.h"

typedef enum {
    PRINT_PREVIEW_ACTION_BACK,
    PRINT_PREVIEW_ACTION_PRINT,
} print_preview_action_t;

typedef void (*print_preview_action_handler_t)(print_preview_action_t action);

void screen_print_preview_set_on_action(print_preview_action_handler_t handler);

// FIXME: the screen_print_preview currently does not copy fpath and fname
// therefore, their lifetime must be at least as long as the screen's lifetime
void screen_print_preview_set_gcode_filepath(const char *fpath);
const char *screen_print_preview_get_gcode_filepath();
void screen_print_preview_set_gcode_filename(const char *fname);
