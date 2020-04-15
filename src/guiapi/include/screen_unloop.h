//screen.h
#pragma once
#include "stdint.h"
#include "screen.h" //screen.t

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    scrn_unlp_timeout,
    scrn_unlp_M876
} screen_unloop_t;

extern void screen_unloop(screen_unloop_t type);

#ifdef __cplusplus
}
#endif //__cplusplus
