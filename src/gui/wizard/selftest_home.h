// selftest_home.h
#ifndef _SELFTEST_HOME_H
#define _SELFTEST_HOME_H

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
    window_progress_t progress;
    window_text_t text_calib_home;
    window_icon_t icon_hourglass;
    uint32_t timer;
} selftest_home_screen_t;

//#pragma pack(1) makes enums 8 bit
typedef struct
{
    _TEST_STATE_t state_home;
} selftest_home_data_t;

#pragma pack(pop)

extern void wizard_init_screen_selftest_home(int16_t id_body, selftest_home_screen_t *p_screen,
    selftest_home_data_t *p_data);

extern int wizard_selftest_home(int16_t id_body, selftest_home_screen_t *p_screen, selftest_home_data_t *p_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SELFTEST_HOME_H
