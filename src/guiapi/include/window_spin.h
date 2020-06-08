// window_spin.h

#ifndef _WINDOW_SPIN_H
#define _WINDOW_SPIN_H

#include "window.h"
#include "window_numb.h"

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

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_spin_t window_class_spin;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_SPIN_H
