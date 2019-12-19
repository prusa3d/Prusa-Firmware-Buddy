// selftest_temp_cool.h
#ifndef _WIZARD_PROGRESS_BAR_H
#define _WIZARD_PROGRESS_BAR_H

#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int wiz_get_percent(int min, int max, int value);
extern int wiz_set_progressbar(window_progress_t *p_progress,
    int min, int max, int value);
extern int wiz_set_progressbar_range(window_progress_t *p_progress,
    int min, int max, int range_min, int range_max, int value);
extern void wiz_set_progressbar_range_auto(window_progress_t *p_progress,
    int min, int max, int range_dif, int value);
extern void wiz_set_progressbar_dual_cl(window_progress_t *p_progress,
    int value, int cl_val, color_t cl_0, color_t cl_100);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WIZARD_PROGRESS_BAR_H
