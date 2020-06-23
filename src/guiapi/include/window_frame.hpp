// window_frame.hpp

#pragma once

#include <inttypes.h>

#include "guitypes.h"
#include "window.hpp"

struct window_class_frame_t {
    window_class_t cls;
};

struct window_frame_t : public window_t {
    color_t color_back;
};

void window_frame_init(window_frame_t *window);
void window_frame_event(window_frame_t *window, uint8_t event, void *param);
void window_frame_done(window_frame_t *window);
void window_frame_draw(window_frame_t *window);
extern const window_class_frame_t window_class_frame;
