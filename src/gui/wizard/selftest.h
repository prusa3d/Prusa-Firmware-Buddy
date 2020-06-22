// selftest.h
#pragma once
#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"
#include "selftest_cool.h"
#include "selftest_temp.h"
#include "selftest_fans_axis.h"

#ifndef _DEBUG
    #define LAST_SELFTEST_TIMEOUT (30 * 60) // [s]
#else
    #define LAST_SELFTEST_TIMEOUT 30 // [s]
#endif                               //_DEBUG

typedef struct
{
    selftest_cool_data_t cool_data;
    selftest_temp_data_t temp_data;
    selftest_fans_axis_data_t fans_axis_data;
} selftest_data_t;

extern uint32_t last_selftest_result;
extern uint32_t last_selftest_time;

extern int wizard_selftest_is_ok(int16_t id_body, selftest_data_t *p_data);
