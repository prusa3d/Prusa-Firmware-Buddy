// selftest_temp.h
#pragma once
#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"

typedef struct
{
    window_text_t text_checking_temp;
    window_progress_t progress;

    uint32_t timer_noz;
    uint32_t timer_bed;

} selftest_temp_screen_t;

typedef struct
{
    _TEST_STATE_t state_preheat_nozzle;
    _TEST_STATE_t state_preheat_bed;
    _TEST_STATE_t state_temp_nozzle;
    _TEST_STATE_t state_temp_bed;
    float temp_noz;
    float temp_bed;
} selftest_temp_data_t;

extern void wizard_init_screen_selftest_temp(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp_nozzle(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp_bed(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);
