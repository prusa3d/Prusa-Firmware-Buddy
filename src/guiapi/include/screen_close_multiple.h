//screen.h
#pragma once
#include "stdint.h"
#include "screen.h" //screen.t

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    scrn_close_on_timeout,
    scrn_close_on_M876
} screen_close_multiple_t;

extern void screen_close_multiple(screen_close_multiple_t type);

#ifdef __cplusplus
}
#endif //__cplusplus
