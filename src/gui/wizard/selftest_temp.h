// selftest_temp.h
#ifndef _SELFTEST_TEMP_H
#define _SELFTEST_TEMP_H

#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_text_t text_checking_temp;
    window_progress_t progress;

    uint32_t timer_noz;
    uint32_t timer_bed;

} selftest_temp_screen_t;

//#pragma pack(1) makes enums 8 bit
typedef struct
{
    _TEST_STATE_t state_preheat_nozzle;
    _TEST_STATE_t state_preheat_bed;
    _TEST_STATE_t state_temp_nozzle;
    _TEST_STATE_t state_temp_bed;
    float temp_noz;
    float temp_bed;
} selftest_temp_data_t;

#pragma pack(pop)

extern void wizard_init_screen_selftest_temp(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp_nozzle(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp_bed(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

extern int wizard_selftest_temp(int16_t id_body, selftest_temp_screen_t *p_screen,
    selftest_temp_data_t *p_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SELFTEST_TEMP_H
