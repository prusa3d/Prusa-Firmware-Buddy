// window_spin.h

#pragma once

#include "window.hpp"
#include "window_numb.hpp"

typedef struct _window_spin_t {
    window_numb_t window;
    float min;
    float max;
    float step;
    int count;
    int index;
} window_spin_t;

typedef struct _window_class_spin_t {
    window_class_numb_t cls;
} window_class_spin_t;

extern const window_class_spin_t window_class_spin;
