//screen.h
#pragma once
#include "stdint.h"

typedef enum {
    scrn_close_on_timeout,
    scrn_close_on_M876
} screen_close_multiple_t;

extern void screen_close_multiple(screen_close_multiple_t type);
