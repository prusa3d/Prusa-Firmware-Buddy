// window_spin.hpp

#pragma once

#include "window.hpp"
#include "window_numb.hpp"

struct window_spin_t : public window_numb_t {
    float min;
    float max;
    float step;
    int count;
    int index;
};

typedef struct _window_class_spin_t {
    window_class_numb_t cls;
} window_class_spin_t;

extern const window_class_spin_t window_class_spin;
