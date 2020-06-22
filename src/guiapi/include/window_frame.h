// window_frame.h

#pragma once

#include <inttypes.h>

#include "guitypes.h"
#include "window.hpp"

typedef struct _window_class_frame_t {
    window_class_t cls;
} window_class_frame_t;

typedef struct _window_frame_t {
    window_t win;
    color_t color_back;
} window_frame_t;

void window_frame_init(window_frame_t *window);
void window_frame_event(window_frame_t *window, uint8_t event, void *param);
void window_frame_done(window_frame_t *window);
void window_frame_draw(window_frame_t *window);
extern const window_class_frame_t window_class_frame;
