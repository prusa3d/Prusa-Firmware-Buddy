//screen.h
#pragma once

#include <inttypes.h>
#include "window.hpp"

typedef struct _screen_t screen_t;

typedef void(screen_init_t)(screen_t *screen);
typedef void(screen_done_t)(screen_t *screen);
typedef void(screen_draw_t)(screen_t *screen);
typedef int(screen_event_t)(screen_t *screen, window_t *window, uint8_t event, void *param);

typedef struct _screen_t {
    int16_t id;            // (2 bytes) screen identifier (2bytes)
    uint32_t flg;          // (4 bytes) flags
    screen_init_t *init;   // (4 bytes) init callback
    screen_done_t *done;   // (4 bytes) done callback
    screen_draw_t *draw;   // (4 bytes) draw callback
    screen_event_t *event; // (4 bytes) event callback
    uint16_t data_size;    // (2 bytes) dynamic data size
    void *pdata;           // (4 bytes) data pointer - automaticaly allocated before init
} screen_t;                // (28 bytes total)

extern int16_t screen_id(void);

extern int16_t screen_register(screen_t *pscreen);

extern screen_t *screen_unregister(int16_t screen_id);

extern void screen_stack_push(int16_t screen_id);

extern int16_t screen_stack_pop(void);

extern void screen_open(int16_t screen_id);

extern void screen_close(void);

extern void screen_draw(void);

extern void screen_dispatch_event(window_t *window, uint8_t event, void *param);

extern screen_t *screen_get_curr(void);
