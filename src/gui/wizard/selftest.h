// selftest.h
#ifndef _SELFTEST_H
#define _SELFTEST_H

#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"
#include "selftest_cool.h"
#include "selftest_temp.h"
#include "selftest_fans_axis.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma pack(push)
#pragma pack(1)

//#pragma pack(1) makes enums 8 bit
typedef struct
{
    selftest_cool_data_t cool_data;
    selftest_temp_data_t temp_data;
    selftest_fans_axis_data_t fans_axis_data;
} selftest_data_t;

#pragma pack(pop)

extern int wizard_selftest_is_ok(int16_t id_body, selftest_data_t *p_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SELFTEST_
