// selftest_fans_axis.h
#pragma once
#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"

//todo create multiple small structures inside this structure
typedef struct
{
    window_progress_t progress_fan;
    window_progress_t progress_axis;
    window_text_t text_fan_test;
    window_text_t text_extruder_fan;
    window_text_t text_print_fan;
    window_text_t text_checking_axis;
    window_text_t text_x_axis;
    window_text_t text_y_axis;
    window_text_t text_z_axis;
    window_icon_t icon_extruder_fan;
    window_icon_t icon_print_fan;
    window_icon_t icon_x_axis;
    window_icon_t icon_y_axis;
    window_icon_t icon_z_axis;
    uint32_t timer0;
    uint32_t timer1;
} selftest_fans_axis_screen_t;

typedef struct
{
    _TEST_STATE_t state_fan0;
    _TEST_STATE_t state_fan1;
    _TEST_STATE_t state_x;
    _TEST_STATE_t state_y;
    _TEST_STATE_t state_z;

    float axis_min[3];
    float axis_max[3];
} selftest_fans_axis_data_t;

extern void wizard_init_screen_selftest_fans_axis(int16_t id_body, selftest_fans_axis_screen_t *p_screen,
    selftest_fans_axis_data_t *p_data);

extern int wizard_selftest_fan0(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data);

extern int wizard_selftest_fan1(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data);

extern int wizard_selftest_x(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data);

extern int wizard_selftest_y(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data);

extern int wizard_selftest_z(int16_t id_body, selftest_fans_axis_screen_t *p_screen, selftest_fans_axis_data_t *p_data);
