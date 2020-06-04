// window_icon.h

#ifndef _WINDOW_ICON_H
#define _WINDOW_ICON_H

#include "window.h"

typedef struct _window_icon_t {
    window_t win;
    color_t color_back;
    uint16_t id_res;
    uint8_t alignment;
} window_icon_t;

typedef struct _window_class_icon_t {
    window_class_t cls;
} window_class_icon_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_icon_t window_class_icon;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WINDOW_ICON_H
