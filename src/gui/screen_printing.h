#pragma once
#include "gui.h"
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void reset_print_state(void); //todo remove me
extern char screen_printing_file_name[_MAX_LFN];
extern char screen_printing_file_path[_MAX_LFN];

extern const screen_t *pscreen_printing;

#ifdef __cplusplus
}
#endif //__cplusplus
