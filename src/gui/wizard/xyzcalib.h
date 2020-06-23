// xyzcalib.h
#pragma once

#include <inttypes.h>
#include "gui.hpp"
#include "wizard_types.h"

struct xyzcalib_screen_t {
    window_progress_t progress;
    window_text_t text_state;
    window_text_t text_search;
    window_icon_t icon;
    uint32_t timer0;
};

struct xyzcalib_data_t {
    _TEST_STATE_t state_home;
    _TEST_STATE_t state_z;
    _TEST_STATE_t state_xy;
    _TEST_STATE_t state_xy_search;
    _TEST_STATE_t state_xy_measure;
};

extern void wizard_init_screen_xyzcalib(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);

extern int xyzcalib_home(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);

extern int xyzcalib_z(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);

extern int xyzcalib_xy_search(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);

extern int xyzcalib_xy_measure(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);

extern int xyzcalib_is_ok(int16_t id_body, xyzcalib_screen_t *p_screen, xyzcalib_data_t *p_data);
