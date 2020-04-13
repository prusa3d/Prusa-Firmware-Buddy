// selftest_cool.h
#ifndef _SELFTEST_COOL_H
#define _SELFTEST_COOL_H

#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"
#include "hwio.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_progress_t progress;
    window_text_t text_waiting_cd;
    window_numb_t target_nozzle;
    window_numb_t target_bed;
    window_icon_t icon_hourglass;
    window_numb_t curr_nozzle_temp;
    window_numb_t curr_bed_temp;

} selftest_cool_screen_t;

//#pragma pack(1) makes enums 8 bit
typedef struct
{
    _TEST_STATE_t state_cool;
    float temp_noz;
    float temp_bed;
    uint32_t timer;
    float start_nozzle_temp;
    float start_bed_temp;
} selftest_cool_data_t;

#pragma pack(pop)

extern void wizard_init_screen_selftest_cool(int16_t id_body, selftest_cool_screen_t *p_screen,
    selftest_cool_data_t *p_data);

extern int wizard_selftest_cool(int16_t id_body, selftest_cool_screen_t *p_screen,
    selftest_cool_data_t *p_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SELFTEST_COOL_H
