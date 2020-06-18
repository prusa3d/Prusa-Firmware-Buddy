// wizard_progress_bar.c

#include "wizard_progress_bar.h"

void wiz_set_progressbar_range_auto(window_progress_t *p_progress,
    int min, int max, int range_dif, int value) {
    int new_min = min - range_dif;
    int new_max = max + range_dif;
    int visible_min = new_min + 3 * (min + max) / 100;

    if (value < visible_min)
        value = visible_min;
    wiz_set_progressbar_range(p_progress, new_min, new_max, min, max, value);
}

int wiz_set_progressbar_range(window_progress_t *p_progress,
    int min, int max, int range_min, int range_max, int value) {
    int percent = wiz_get_percent(min, max, value);
    if (value < range_min)
        p_progress->color_progress = COLOR_BLUE;
    else if (value > range_max)
        p_progress->color_progress = COLOR_RED;
    else
        p_progress->color_progress = COLOR_LIME;
    window_set_value(p_progress->win.id, percent);
    return percent;
}

int wiz_set_progressbar(window_progress_t *p_progress,
    int min, int max, int value) {
    int percent = wiz_get_percent(min, max, value);
    if (percent >= 100)
        p_progress->color_progress = COLOR_LIME;
    else
        p_progress->color_progress = COLOR_BLUE;
    window_set_value(p_progress->win.id, percent);
    return percent;
}

int wiz_get_percent(int min, int max, int value) {
    if (value <= min)
        return 0;
    if (value >= max)
        return 100;

    return (float)(value - min) * 100.F / (float)(max - min);
}

static void mix_cl(color_t *ret, color_t cl_0, color_t cl_100, int progress, int chan) {
    color_t cl;
    cl_0 = (cl_0 >> (chan * 8)) & 0xff;
    cl_100 = (cl_100 >> (chan * 8)) & 0xff;

    cl = ((100 - progress) * cl_0) / 100;
    cl += (progress * cl_100) / 100;
    if (cl > 0xff)
        cl = 0xff;

    *ret |= cl << (chan * 8);
}

void wiz_set_progressbar_dual_cl(window_progress_t *p_progress,
    int value, int cl_val, color_t cl_0, color_t cl_100) {
    if (value > 100)
        value = 100;
    if (value < 0)
        value = 2; //visible minimum

    color_t color = 0;
    for (int channel = 0; channel <= 3; ++channel) {
        mix_cl(&color, cl_0, cl_100, cl_val, channel);
    }

    p_progress->color_progress = color;
    window_set_value(p_progress->win.id, value);
}
