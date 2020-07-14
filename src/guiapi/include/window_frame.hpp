// window_frame.hpp

#pragma once

#include <inttypes.h>

#include "guitypes.h"
#include "window.hpp"
#include "gui.hpp"

struct window_class_frame_t {
    window_class_t cls;
};

struct window_frame_t : public window_t {
    window_t *first;
    void SetFirst(window_t *fir);
    window_frame_t(window_t *parent = nullptr, window_t *prev = nullptr, rect_ui16_t rect = rect_ui16(0, 0, display::GetW(), display::GetH()));
    virtual void Draw();
    virtual int Event(window_t *sender, uint8_t event, void *param);
};

void window_frame_init(window_frame_t *window);
void window_frame_event(window_frame_t *window, uint8_t event, void *param);
void window_frame_done(window_frame_t *window);
void window_frame_draw(window_frame_t *window);
extern const window_class_frame_t window_class_frame;
